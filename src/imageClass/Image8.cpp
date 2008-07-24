#include "Image8.h"

Image8::Image8(unsigned int height, unsigned int width, unsigned int channel, bool zero_image) : height(height), width(width), channel(channel), size(height*width*channel)
{
	data = new unsigned char [size];

	if (zero_image)
		zero();
}

Image8::~Image8()
{
	delete [] data;
}
