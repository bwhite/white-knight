#include <IL/il.h>
#include <cstdlib>
#include <iostream>
#include "devil.h"

Devil::Devil()
{
	ilInit();
	//TODO Use ifdef's to confirm this
	handledExtensions.push_back("jpeg");
	handledExtensions.push_back("jpg");
	handledExtensions.push_back("bmp");
	handledExtensions.push_back("png");
}

Devil::~Devil()
{
	
}

bool Devil::getImageHeader(unsigned int &height, unsigned int &width, unsigned int &channels, const char * fName)
{
	ILuint ImageName;
    ilGenImages(1, &ImageName);
    ilBindImage(ImageName);

    bool returnVal = ilLoadImage((char *)fName);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	width = ilGetInteger(IL_IMAGE_WIDTH);
	channels = ilGetInteger(IL_IMAGE_CHANNELS);
	ilDeleteImages(1, &ImageName);
	//cout << "DVL: Height - " << height << " Width - " << width << " Channels - " << channels << endl;
	return returnVal;
}

bool Devil::readImage(Image8 &image, const char * fName)
{
	ILuint ImageName;
    ilGenImages(1, &ImageName);
    ilBindImage(ImageName);
	ilEnable(IL_ORIGIN_SET);
	ilSetInteger(IL_ORIGIN_MODE, IL_ORIGIN_UPPER_LEFT);
    bool returnVal = ilLoadImage((char *)fName);

	unsigned int height = ilGetInteger(IL_IMAGE_HEIGHT);
	unsigned int width = ilGetInteger(IL_IMAGE_WIDTH);
	unsigned int channels = ilGetInteger(IL_IMAGE_CHANNELS);
	unsigned int bytesPerPixel = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	assert(returnVal);
	assert(image.getHeight() == height);
	assert(image.getWidth() == width);
	assert(image.getChannels() == channels);
	assert(image.getChannels() == bytesPerPixel);
	
	ILubyte *Data = ilGetData();

	unsigned char * imageData = image.getDataPointer();

	memcpy(imageData, Data, height*width*channels);

	ilDeleteImages(1, &ImageName);
	
	return returnVal;
}

bool Devil::writeImage(Image8 &image, const char * fName)
{
	ILuint ImageName;
    ilGenImages(1, &ImageName);
    ilBindImage(ImageName);
	unsigned int channels = image.getChannels();

	ILenum imageFormat;
	if (channels == 3)
		imageFormat = IL_RGB;
	else if (channels == 1)
		imageFormat = IL_LUMINANCE;
	else
	{
		cerr << "Unknown image format!" << endl;
		exit(1);
	}

	ilEnable(IL_FILE_OVERWRITE);

	ilTexImage(image.getWidth(), image.getHeight(), 1, channels,imageFormat, IL_UNSIGNED_BYTE,0);
	ilRegisterOrigin(IL_ORIGIN_UPPER_LEFT);
	ilSetData(image.getDataPointer()); 
	bool returnVal = ilSaveImage((char *)fName);//Unfortunately they don't use const char *
	assert(returnVal);
	
	ilDeleteImages(1, &ImageName);
	return returnVal;
}
