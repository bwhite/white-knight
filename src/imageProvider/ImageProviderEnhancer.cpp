#include "../imageTools/imageTools.h"

ImageProviderEnhancer::ImageProviderEnhancer(list<vector<string> > configList, ImageProvider *realProvider) : realProvider(realProvider), workerThreadRunning(false)
{
	assert(realProvider);
	int notLoaded = 0;
	int tempImageResize = 0;
	tempImageResize += FindValue(configList, "ImageEnhResizeWidth", resizeWidth);
	tempImageResize += FindValue(configList, "ImageEnhResizeHeight", resizeHeight);
	imageResizing = !tempImageResize;
	
	assert(!notLoaded);
}

ImageProviderEnhancer::~ImageProviderEnhancer()
{
	
}

///Returns our internal height/width/channel
void ImageProviderEnhancer::getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels)
{
	realImage->getImageInfo(height,width,channels);
	if (imageResizing)
	{
		height = this->resizeHeight;
		width = this->resizeWidth;
	}
}

///Given an image that matches our internal height/width/channels, we will take the next file in our list, find its extension handler, and load the file
void ImageProviderEnhancer::getImage (Image8 &image)
{
	//TODO Pull image from buffer
	if (imageResizing)
	{
		
	}
}

///Erases all current state information, which can have different effects depending on the underlying implementation
void ImageProviderEnhancer::reset ()
{
	//TODO Join worker thread(If running), clear buffer, reset realProvider, restart worker thread
	
}