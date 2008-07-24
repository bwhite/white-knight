/**
 * This class takes in another ImageProvider, and can do frame buffering (in a separate thread to speed up image requests from the client), image pre-processing (contrast enhancement, un-auto-gain for self-adjusting cameras, image rotation, cropping, blurring, etc), and image size/channel conversion (color->gray, bilinear downsample/upsample).
 *
 * \author Brandyn White
 */
#ifndef IMPROV_ENH_H
#define IMPROV_ENH_H

#include <list>

#include "../ImageProvider.h"
#include "../imageClass/Image8.h"
#include "../configTools/configReader.h"

using namespace std;

class ImageProviderEnhancer : public ImageProvider
{
public:
	ImageProviderEnhancer(list<vector<string> > configList, ImageProvider *realProvider);
	virtual ~ImageProviderEnhancer();

	///Returns our internal height/width/channel
	void getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels);
	///Given an image that matches our internal height/width/channels, we will take the next file in our list, find its extension handler, and load the file
	void getImage (Image8 &image);

	///Erases all current state information, which can have different effects depending on the underlying implementation
	void reset ();
	
private:
	unsigned int height;
	unsigned int width;
	unsigned int channels;
	ImageProvider *realProvider;
	
	bool imageBuffering;
	bool workerThreadRunning;
	unsigned int bufferSize;
	
	bool imageResizing;//Takes a predefined size and resizes all incoming images to that
	unsigned int resizeWidth;
	unsigned int resizeHeight;
};

void * ImageBufferThread(void * args);

#endif
