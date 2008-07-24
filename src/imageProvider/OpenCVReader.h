#ifndef _OPENCVREADER_H_
#define _OPENCVREADER_H_
#include <list>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "../ImageProvider.h"
#include "../imageClass/Image8.h"
#include "../ImageFormat.h"
#include "../configTools/configReader.h"
#include "../imageFormat/devil.h"

using namespace std;

class OpenCVReader : public ImageProvider
{
public:
	OpenCVReader(list<vector<string> > configList);
	~OpenCVReader();
	virtual void getImage (Image8 &image) throw (NoMoreImages);
	virtual void getImage (Image8 *image) throw (NoMoreImages);
	virtual void reset ();//Effectively erases all current state information and can be treated as newly instantiated
	virtual void getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels);
private:
	void cvImageToImage8(IplImage *cvImage, Image8 &image);
	char fileName[100];
	int cameraNumber;
	CvCapture* cvImageReader;
	bool cameraInput;
	unsigned int height;
	unsigned int width;
	unsigned int channels;
};
#endif /* _OPENCVREADER_H_ */
