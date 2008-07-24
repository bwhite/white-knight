#ifndef DEVIL_H
#define DEVIL_H
#include "../imageClass/Image8.h"
#include "../ImageFormat.h"

class Devil : public ImageFormat
{
public:
	Devil();
	virtual ~Devil();
	bool getImageHeader(unsigned int &height, unsigned int &width, unsigned int &channels, const char * fName);
	bool readImage(Image8 &image, const char * fName);
	bool writeImage(Image8 &image, const char * fName);
};
#endif
