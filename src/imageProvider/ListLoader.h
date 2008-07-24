/**
 * This class allows for a list of images to be provided to the caller, the height, width, and number of channels must all be the same between all of the files in the list; however, the actual file type doesn't matter as long as there is an image format that handles it
 *
 * \author Brandyn White
 */
#ifndef LIST_LOADER_H
#define LIST_LOADER_H

#include <list>

#include "../ImageProvider.h"
#include "../imageClass/Image8.h"
#include "../ImageFormat.h"
#include "../configTools/configReader.h"

using namespace std;

class ListLoader : public ImageProvider
{
public:
	ListLoader(list<vector<string> > configList, list<ImageFormat*> formats, char* fileListName = 0);
	virtual ~ListLoader();

	///Returns our internal height/width/channel
	void getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels);

	///Updated the number of listed images
	void updateNumListedImages();

	///Given an image that matches our internal height/width/channels, we will take the next file in our list, find its extension handler, and load the file
	void getImage (Image8 &image) throw (NoMoreImages);
	void getImage (Image8 *image) throw (NoMoreImages);
	///Erases all current state information, which can have different effects depending on the underlying implementation
	void reset ();

	///Displays the frame number of the image you will get if you call getImage
	unsigned int getFrameCount()
	{
		return frameCounter;
	}

	unsigned int getNumListedImages()
	{
		return numListedImages;
	}
	
	unsigned int getNumImagesRemaining()
	{
		return numListedImages - returnedImages;
	}
	
protected:
	ImageFormat* getFileFormat(const char *fileName);
	list<ImageFormat*> formats;
	char fileListName[100];
	char inputDir[100];
	unsigned int lineSkip;
	unsigned int initialFrameNumber;
	unsigned int height;
	unsigned int width;
	unsigned int channels;
	unsigned int numListedImages;
	
	//Variables that represent state of the current image (important as we want to be able to save/load these)
	unsigned int returnedImages;
	unsigned int frameCounter;
	FILE *fileList;
	
	//These should match the ones above less the stateSaved one
	bool stateSaved;
	unsigned int savedReturnedImages;
	unsigned int savedFrameCounter;
	FILE *savedFileList;
};
#endif
