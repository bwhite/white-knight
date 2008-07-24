///This class is an abstract base class for image file reading.  This can be used to wrap a single file type
///and list the extensions it uses, or it can wrap a much larger type (it could theoreticaly wrap a list of its same type) such as openCV
///or a movie reader
///\author Brandyn White
#ifndef IMAGE_FORMAT_H
#define IMAGE_FORMAT_H
#include <list>
#include <strings.h>
#include <iostream>

using namespace std;

class ImageFormat
{
public:
	virtual bool getImageHeader(unsigned int &height, unsigned int &width, unsigned int &channels, const char * fName) = 0;
	virtual bool readImage(Image8 &image, const char * fName) = 0;
	virtual bool writeImage(Image8 &image, const char * fName) = 0;
	virtual ~ImageFormat()
	{}
	///Returns true if the image format handles the given extension and size, if the size is 0, than it is ignored which is necessary for header reading
	bool isHandled(const char* extension, const unsigned int size = 0)
	{
		bool handlesExtension = false;

		for (list<char*>::iterator extensionIter = handledExtensions.begin(); extensionIter != handledExtensions.end(); extensionIter++)
		{
			if (!strcasecmp(*extensionIter,extension))
			{
				handlesExtension = true;
				break;
			}
		}

		if (!handlesExtension)
			return false;

		if (!size)
			return true;

		for (list<unsigned int>::iterator channelIter = handledChannelSizes.begin(); channelIter != handledChannelSizes.end(); channelIter++)
		{
			if (*channelIter == size)
				return true;
		}
		return false;
	}
protected:
	list<char*> handledExtensions;
	list<unsigned int> handledChannelSizes;
	void errorCantOpenFile(const char * fName)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - Can't open file [" << fName << "]!" << endl;
	}
};
#endif
