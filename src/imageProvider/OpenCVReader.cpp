#include "OpenCVReader.h"

OpenCVReader::OpenCVReader(list<vector<string> > configList)
{
	int notLoaded = 0;
	int tempCameraInput = 0;
	notLoaded += FindValue(configList, "CVCameraInput", tempCameraInput);
	assert(!notLoaded);
	cameraInput = (tempCameraInput != 0);
	if (!cameraInput)
	{
		notLoaded += FindValue(configList, "CVMovieFileInput", fileName);
		assert(!notLoaded);
		cvImageReader = cvCaptureFromFile(fileName);
	}
	else
	{
		notLoaded += FindValue(configList, "CVCameraNumber", cameraNumber);
		assert(!notLoaded);
		cvImageReader = cvCaptureFromCAM(cameraNumber);
	}
	assert(cvImageReader);
	IplImage* cvImage = cvQueryFrame(cvImageReader);
	assert(cvImage);
	height = cvImage->height;
	width = cvImage->width;
	channels = cvImage->nChannels;
	reset();
	//cout << "Height: " << height << " Width: " << width << " Channels: " << channels << endl;
}

OpenCVReader::~OpenCVReader()
{
	cvReleaseCapture(&cvImageReader);
}

void OpenCVReader::getImage (Image8 &image) throw (NoMoreImages)
{
	assert(image.getHeight() == height);
	assert(image.getWidth() == width);
	assert(image.getChannels() == channels);
	
	IplImage* cvImage = cvQueryFrame(cvImageReader);
	if (!cvImage)
		throw NoMoreImages("No more images provided by OpenCV");
	cvImageToImage8(cvImage, image);
}

void OpenCVReader::getImage (Image8 *image) throw (NoMoreImages)
{
	if (image)
		getImage(*image);
}

void OpenCVReader::reset ()
{
	cvReleaseCapture(&cvImageReader);
	if (!cameraInput)
	{
		cvImageReader = cvCaptureFromFile(fileName);
	}
	else
	{
		cvImageReader = cvCaptureFromCAM(cameraNumber);
		for (unsigned int frameSkip = 0; frameSkip < 5; frameSkip++)
			cvQueryFrame(cvImageReader);
	}
}

void OpenCVReader::getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels)
{
	height = this->height;
	width = this->width;
	channels = this->channels;
}

void OpenCVReader::cvImageToImage8(IplImage *cvImage, Image8 &image)
{
	assert(channels == 3);
	unsigned char *imageData = image.getDataPointer();
	for (unsigned int pixelCounter = 0; pixelCounter < height*width; pixelCounter++)//Number of pixels
	{
		for (unsigned int chanIter = 0; chanIter < 3; chanIter++)
			imageData[pixelCounter*3 + chanIter] = cvImage->imageData[pixelCounter*3 + (2-chanIter)];//BGR to RGB
	}
}
