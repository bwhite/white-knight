/**
 * This class is an abstract base class that allows for generic image input
 *
 * \author Brandyn White
 */

#ifndef IMAGE_PROVIDER_H
#define IMAGE_PROVIDER_H
#include "imageClass/Image8.h"
#include <exception>
#include <string>

class NoMoreImages : public std::exception
{ 
public: 
	NoMoreImages(const std::string& m) : msg(m) {} 
	
	~NoMoreImages () throw () {}
	const char* what() const throw()
	{ 
		return msg.c_str(); 
	}
	private:
		std::string msg; 
};

class ImageProvider
{
public:
	virtual void getImage (Image8 &image) throw (NoMoreImages) = 0;
	virtual void getImage (Image8 *image) throw (NoMoreImages) = 0;//Same as the reference version;however, if you pass NULL/0 it will skip that image
	virtual void reset () = 0;//Effectively erases all current state information and can be treated as newly instantiated
	virtual void getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels) = 0;
	virtual ~ImageProvider()
	{}
}
;
#endif
