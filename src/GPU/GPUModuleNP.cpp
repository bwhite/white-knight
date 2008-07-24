#include "GPUModuleNP.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

//TODO These includes are system dependant so we should use autoconf or the like to configure them
//#include <glut/glut.h>
#ifndef MAC
#include <GL/glew.h>
#include <GL/glut.h>
#endif
#ifdef MAC
#include <glut/glut.h>
#endif
// These sure look like constants, but they're not!
int HEIGHT;
int WIDTH;
int WINDOW;

GLchar sourcecode[65536];
const GLchar *sourcecode_ptr = sourcecode;

void InitTexture(GLuint &id)
{		
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, id);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, 
					WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, 0);		
}

// The offscreen framebuffer
static GLuint fb;


// Would relieve some tedious initializing pressure if I wanted more than one
class ShaderProgram
{
public:
	GLuint p;
	GLuint s;
	
	void Init(const char *filename)
	{
		FILE *file = fopen(filename,"r");
		if (!file)
		{
			printf("Couldn't open file: '%s'\n", filename);
			exit(1);
		}

		fread(sourcecode, sizeof(sourcecode), sizeof(GLchar), file);
	
		if (!feof(file)) 
		{
			printf("File was too big!!\n");
			exit(1);
		}
		fclose(file);
		
		p = glCreateProgram();
		s = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(s, 1,  &sourcecode_ptr, 0);
		glCompileShader(s);
		glAttachShader(p, s);
		glLinkProgram(p);
		glValidateProgram(p);	
	}
};
// UpdateGaussian.glsl
static ShaderProgram accumulator;

// Input variables to the shader program

static GLint glslAccumulator_pixel;	
static GLint glslAccumulator_point;	
static GLint glslAccumulator_window;
static GLint glslAccumulator_accum;	
static GLint glslAccumulator_var;

// Ring buffer of textures for the sliding window (and the current image) 
static GLuint *buffer;
static int buffer_current;
static int buffer_size;

static GLuint accum, accum_back;

// Handle returned by GLUT
static int window;

static void ping_pong(GLuint &ping, GLuint &pong)
{
	GLuint tmp = ping;
	ping = pong;
	pong = tmp;
}

// Check for errors of any kind
static void dumplog()
{
	GLchar log[65536];
	GLsizei len;
	glGetShaderInfoLog(accumulator.s, 65536, &len, log);
	if (len)
		printf("%s\n", log);
	glGetProgramInfoLog(accumulator.p, 65536, &len, log);
	if (len)
		printf("%s\n", log);
	int glError = glGetError();
	if (glError != GL_NO_ERROR)
		printf("%s\n", gluErrorString(glError));
}

static void display()
{

}

// Fill the buffer with the initial images
void GPUModuleNP::FillBuffer(const float *in)
{
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, buffer[buffer_current]);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, 
		GL_FLOAT, in);		
				
	buffer_current = (buffer_current + 1) % (WINDOW + 1);
}

void GPUModuleNP::Subtract(const float *in, float *out, float v1, float v2, float v3, 
						bool update)
{
	// Load the new image into a texture
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, buffer[buffer_current]);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, 
		GL_FLOAT, in);
	
	// Clear the accumulation buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_ARB, accum_back, 0);
	glClearIndex(0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	// Accumulate the probability for each texture in the window
	for (int i = 0; i < WINDOW ; i++)
	{	
		int index = (buffer_current + 1 + i) % (WINDOW + 1);
		

		// Bind the framebuffer to the accumulation texture
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 			
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, accum, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		
		
		// Setup accumulator
		glUseProgram(accumulator.p);
		
		glActiveTexture(GL_TEXTURE0); 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, accum_back);
		glUniform1i(glslAccumulator_accum, 0);
		
		glActiveTexture(GL_TEXTURE1); 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, buffer[index]);
		glUniform1i(glslAccumulator_point, 1);
		 
		glActiveTexture(GL_TEXTURE2); 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, buffer[buffer_current]);
		glUniform1i(glslAccumulator_pixel, 2);
		
		glUniform1f(glslAccumulator_window, WINDOW);
		glUniform3f(glslAccumulator_var, v1, v2, v3);
	
		// Accumulate!	
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glViewport(0, 0, WIDTH, HEIGHT);
		gluOrtho2D(0, WIDTH, 0, HEIGHT);
		
	
		glBegin(GL_TRIANGLES);
		glVertex2f(0.0, 0.0);
		glVertex2f(2.0*WIDTH, 0.0);
		glVertex2f(0.0, 2.0*HEIGHT);
		glEnd();
		
		// Cleanup
		glUseProgram(0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		
		// Ping Pong!
		ping_pong(accum, accum_back);
	}
	
	// (Optionally) Update the buffer to include the newest image
	if (update)
		buffer_current = (buffer_current + 1) % (WINDOW + 1);

	// Give the probability back to the caller
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, accum_back);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_ARB, accum_back, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, out);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

// Set up everything
void GPUModuleNP::Init(unsigned int window, 
	unsigned int height, unsigned int width)
{
	HEIGHT = height;
	WIDTH = width;
	WINDOW = window;
	
	
	char dummy[] = {'\0'};
	char *dummyv = dummy;
	int dummyc = 0;
	
	glutInit(&dummyc, &dummyv);
	glutInitDisplayMode(GLUT_DOUBLE);
	//glutInitWindowSize(256,256);
	//glutInitWindowPosition(400, 100);
	window = glutCreateWindow("GPU Computer Vision");	
	
	#ifndef MAC
	if (glewInit() != GLEW_OK)
	{
		//cerr << "GLEW Failed!" << endl;
		exit(1);
	}
	#endif
	glutDisplayFunc(display);
	
	
	// NOTE: Change this according to your build tree
	accumulator.Init("src/GPU/accumulate.glsl");

	glslAccumulator_window = glGetUniformLocation(accumulator.p, "window");
	glslAccumulator_accum = glGetUniformLocation(accumulator.p, "accum_sampler");
	glslAccumulator_point = glGetUniformLocation(accumulator.p, "point_sampler");
	glslAccumulator_pixel = glGetUniformLocation(accumulator.p, "pixel_sampler");
	glslAccumulator_var = glGetUniformLocation(accumulator.p, "var");
	
	
	
	buffer = new GLuint[WINDOW + 1];
	buffer_current = 0;
	buffer_size = 0;
	
	for (int i = 0; i < WINDOW + 1; i++)
	{
		InitTexture(buffer[i]);
	}
	

	// Accumulation buffers
	InitTexture(accum);
	InitTexture(accum_back);

	// Bind this texture to a framebuffer which can be rendered to directly
	glGenFramebuffersEXT(1, &fb);

	dumplog();
}

void GPUModuleNP::EventLoop()
{
	glutMainLoop();
}

void GPUModuleNP::Destroy()
{
	glutDestroyWindow(window);
}
