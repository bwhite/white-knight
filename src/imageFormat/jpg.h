#ifndef JPG_H
#define JPG_H
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <cstdio>
#include "../imageClass/Image8.h"
#include "../ImageFormat.h"

#define MAX_LINE 100
class JPEG : public ImageFormat
{
public:
	JPEG();
	virtual ~JPEG();
	bool getImageHeader(unsigned int &height, unsigned int &width, unsigned int &channels, const char * fName);
	bool readImage(Image8 &image, const char * fName);
	bool writeImage(Image8 &image, const char * fName);
};
#endif
