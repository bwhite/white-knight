#include "NonParametricGPU.h"
#include "../GPU/GPUModuleNP.h"
#include "../imageFormat/devil.h"
//Helper functions for conversion

//NOTE size = height * width    the float is assumed to have 4 channels, and the image 3!
//Below was too slow, so we will try again
/*void Image8ToFloat(Image8 &input, float *output, unsigned int size)
{
	unsigned char *inputData = input.getDataPointer();
	unsigned int image8Iter = 0;
	unsigned int floatIter = 0;
	for (unsigned int iter = 0; iter < size; iter++)
		for (unsigned int chanIter = 0; chanIter < 4; chanIter++)
		{
			if (chanIter < 3)
				output[floatIter] = (float)inputData[image8Iter++];
			floatIter++;
		}
}*/
void Image8ToFloat(Image8 &input, float *output, unsigned int size)
{
	unsigned char *inputData = input.getDataPointer();
	unsigned int image8Iter = 0;
	unsigned int floatIter = 0;
	for (unsigned int iter = 0; iter < size; iter++)
	{
		for (unsigned int chanIter = 0; chanIter < 3; chanIter++)
			output[floatIter++] = inputData[image8Iter++];
		floatIter++;
	}
}


//NOTE size = height * width    the float is assumed to have 4 channels, and the image 3!
void FloatToImage8Gray(float *input, Image8 &output,unsigned int size)
{		
	unsigned char *outputData = output.getDataPointer();
	unsigned int image8Iter = 0;
	unsigned int floatIter = 0;
	for (unsigned int iter = 0; iter < size; iter++)
		for (unsigned int chanIter = 0; chanIter < 4; chanIter++)
		{
			if (chanIter < 1)
				outputData[image8Iter++] = (unsigned char)input[floatIter];
			floatIter++;
		}
}
bool glutInitialized = false;
NonParametricGPU::NonParametricGPU(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, list<vector<string> > configList):height(height), width(width), channels(channels)
{
	int notLoaded = 0;
	notLoaded += FindValue(configList, "GPUNonParamWindowSize", windowSize);
	notLoaded += FindValue(configList, "GPUNonParamSphereVariance", sphereVariance);
	notLoaded += FindValue(configList, "GPUNonParamBackgroundThreshold", backgroundThreshold);
	notLoaded += FindValue(configList, "GPUNonParamFrameSkip", frameSkip);
	if (notLoaded != 0)
		exit(0);
	if (!glutInitialized)
	{
		glutInitialized = true;
		GPUModuleNP::Init(windowSize, height, width);
	}
	Image8 currentImage(height,width,channels);
	currentGPUImage = new float[height*width*4];
	outputImage = new float[height*width*4];
	this->imProvide = imProvide;

	for (unsigned int frameIter = 0; frameIter < windowSize; frameIter++)
	{
		imProvide->getImage(currentImage);
		Image8ToFloat(currentImage,currentGPUImage, height*width);
		GPUModuleNP::FillBuffer(currentGPUImage);
	}
	imProvide->reset();
}

NonParametricGPU::~NonParametricGPU()
{
	GPUModuleNP::Destroy();
	delete [] currentGPUImage;
	delete [] outputImage;
}

void NonParametricGPU::subtract (Image8 &differenceImage, Image8 &imageRan)
{
	assert(differenceImage.getHeight() == height);
	assert(differenceImage.getWidth() == width);
	
	//TODO Do sanity checks that everything that should equal is
	//TODO Use both image volumes (large and small) as is in the paper
	differenceImage.zero();
	imProvide->getImage(imageRan);
	static unsigned int curFrame = 0;	
	
	Image8ToFloat(imageRan,currentGPUImage, height*width);
	GPUModuleNP::Subtract(currentGPUImage, outputImage, sphereVariance, sphereVariance, sphereVariance,curFrame % frameSkip == 0);
		
	for (unsigned int outputIter = 0; outputIter < 4*width*height; outputIter++)
	{
		if (outputImage[outputIter] < backgroundThreshold)
			outputImage[outputIter] = 255;
		else
			outputImage[outputIter] = 0;
	}
	
	FloatToImage8Gray(outputImage,differenceImage,height*width);
}
