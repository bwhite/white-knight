#include "GPUModule.h"

#include <cstdio>
#include <cstring>
#include <cassert>

#include <glut/glut.h>

// Relieves some tedious naming and intializing pressures
class Texture
{
private:
	static GLenum _current_unit;
public:
	void Init(GLenum format1, GLenum format2)
	{
		this->format1 = format1;
		this->format2 = format2;
		
		glGenTextures(1, &id);
		unit = _current_unit++;
		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, id);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, format1, GPUModule::WIDTH, GPUModule::HEIGHT, 0, format2, GL_FLOAT, 0);		
	}
	
	GLuint id;
	GLenum unit;
	GLenum format1;
	GLenum format2;
};
GLenum Texture::_current_unit = GL_TEXTURE0;

// These hold the input and output data
static Texture onetex, twotex, finaltex;

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
		GLchar sourcecode[65536];
		const GLchar *sourcecode_ptr = sourcecode;
		
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
static ShaderProgram program;

// Input variables to the shader program
static GLint glslOne;
static GLint glslTwo;
static GLint glslAlpha;


// Handle returned by GLUT
static int window;

// Check for errors of any kind
static void dumplog()
{
	GLchar log[65536];
	GLsizei len;
	glGetShaderInfoLog(program.s, 65536, &len, log);
	if (len)
		printf("%s\n", log);
	glGetProgramInfoLog(program.p, 65536, &len, log);
	if (len)
		printf("%s\n", log);
	int glError = glGetError();
	if (glError != GL_NO_ERROR)
		printf("%s\n", gluErrorString(glError));
}

void display()
{

}

// Set up everything
void GPUModule::Init(unsigned int height, unsigned int width)
{
	char dummy[] = {'\0'};
	char *dummyv = dummy;
	int dummyc = 0;
	HEIGHT = height;
	WIDTH = width;
	
	glutInit(&dummyc, &dummyv);
	glutInitDisplayMode(GLUT_DOUBLE);
	glutInitWindowSize(256,256);
	//glutInitWindowPosition(400, 100);
	window = glutCreateWindow("GPU Computer Vision");
	
	glutDisplayFunc(display);


	onetex.Init(GL_RGBA32F_ARB, GL_RGBA);
	twotex.Init(GL_RGBA32F_ARB, GL_RGBA);
	finaltex.Init(GL_RGBA32F_ARB, GL_RGBA);
	
	// Bind this texture to a framebuffer which can be rendered to directly
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, finaltex.id, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// NOTE: Change this according to your build tree
	program.Init("src/GPU/updategaussian.glsl");
	
	glslOne = glGetUniformLocation(program.p, "one_sampler");
	glslTwo = glGetUniformLocation(program.p, "two_sampler");
	glslAlpha = glGetUniformLocation(program.p, "alpha");
	
	dumplog();
}

void GPUModule::UpdateGaussian(unsigned int height, float alpha, const float *inone, const float *intwo, float *out)
{
	glActiveTexture(onetex.unit);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, WIDTH, height, GL_RGBA, GL_FLOAT, inone);
	
	glActiveTexture(twotex.unit);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, WIDTH, height, GL_RGBA, GL_FLOAT, intwo);


	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, WIDTH, HEIGHT);
	gluOrtho2D(0, WIDTH, 0, HEIGHT);
	
	glUseProgram(program.p);
	glUniform1i(glslOne, onetex.unit - GL_TEXTURE0);
	glUniform1i(glslTwo, twotex.unit - GL_TEXTURE0);
	glUniform1f(glslAlpha, alpha);
	
	glBegin(GL_QUADS);
	glVertex2f(0.0, 0.0);
	glVertex2f(WIDTH, 0.0);
	glVertex2f(WIDTH, HEIGHT);
	glVertex2f(0.0, HEIGHT);
	glEnd();
	
	glUseProgram(0);
	
	glReadPixels(0, 0, WIDTH, height, GL_RGBA, GL_FLOAT, out);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glFinish();
	
	
	
	
	dumplog();
}

void GPUModule::EventLoop()
{
	glutMainLoop();
}

void GPUModule::Destroy()
{
	glutDestroyWindow(window);
}