#include "NonParametric.h"

double bandR;
double bandG;
double bandB;

///The constructor takes the config list from which variables can be changed, and the image provider that we will call to get the data we need.
///The height and width come from the image provider and are initialized as constants
unsigned int NonParametricPixel::channels;//MUST initialize with setChannels before running, and must never be changed!
unsigned int NonParametricPixel::publicSize;//MUST initialize with setChannels before running!
NonParametric::NonParametric(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, std::list<vector<string> > configList) : height(height), width(width), channels(channels)
{
	imProvide->reset();//We want to start off in a predictable state, as we need to use this other times we want consistency
	//TODO Initialize config variables

	int notLoaded = 0;
	notLoaded += FindValue(configList, "SelectiveWindowSize", selectiveWindowSize);
	notLoaded += FindValue(configList, "NonselectiveWindowSize", nonselectiveWindowSize);
	notLoaded += FindValue(configList, "MinBackgroundThresh", minBackgroundThreshold);
	notLoaded += FindValue(configList, "DisplacementThresh", dispThresh);
	notLoaded += FindValue(configList, "CCDisplacementThresh", ccDispThresh);
	notLoaded += FindValue(configList, "NeighborhoodRadius", neighborhoodRadius);
	notLoaded += FindValue(configList, "NonParamBandR", bandR);
	notLoaded += FindValue(configList, "NonParamBandG", bandG);
	notLoaded += FindValue(configList, "NonParamBandB", bandB);
	if (notLoaded)
		exit(1);
	
	NonParametricPixel::setChannels(channels);
	
	NonParametricPixel::setSize(selectiveWindowSize);
	pixMaps[0] = new NonParametricPixel[height*width];
	
	NonParametricPixel::setSize(nonselectiveWindowSize);
	pixMaps[1] = new NonParametricPixel[height*width];
	pixDispProbMap = new double[height*width];
	tempDiffImages[0] = new Image8(height, width, 1);
	tempDiffImages[1] = new Image8(height, width, 1);
	
	this->imProvide = imProvide;

	//Initialize buffers
	Image8 *currentImage = new Image8(height,width,channels);
	double *pixel = new double[3];
	
	unsigned int maxSize = selectiveWindowSize;
	if (nonselectiveWindowSize > maxSize)
		maxSize = nonselectiveWindowSize;
	
	//TODO Check to see if we have enough frames to even fill out buffer, if not use however many we can get
	for (unsigned int frameLoaderIter = 0; frameLoaderIter < maxSize; frameLoaderIter++)
	{
		imProvide->getImage(*currentImage);
		for (unsigned int yIter = 0; yIter < height; yIter++)
			for (unsigned int xIter = 0; xIter < width; xIter++)
			{
				//Do color conversions here
				pixel[0] = currentImage->read(yIter, xIter, 0);
				pixel[1] = currentImage->read(yIter, xIter, 1);
				pixel[2] = currentImage->read(yIter, xIter, 2);
				pixMaps[0][yIter*width + xIter].sampleInitialize(pixel);
				pixMaps[1][yIter*width + xIter].sampleInitialize(pixel);
			}
	}
	delete [] pixel;
	delete currentImage;
	
	//Reset image provider so that when we do subtraction we start over again, both with frame numbers, and depending on what the image provider is, we could reset a list and recieve the images again in subtraction
	imProvide->reset();
	
	for (unsigned int yIter = 0; yIter < height; yIter++)
		for (unsigned int xIter = 0; xIter < width; xIter++)
		{	
			pixMaps[0][yIter*width + xIter].calculateBandwidth();
			pixMaps[1][yIter*width + xIter].setBandwidth(pixMaps[0][yIter*width + xIter].getBandwidth());
		}
}

NonParametric::~NonParametric()
{
	delete [] pixDispProbMap;
	delete [] pixMaps[0];
	delete [] pixMaps[1];
	delete tempDiffImages[0];
	delete tempDiffImages[1];
}

//TODO for things like this we have to decide which list to use!

double NonParametric::getCCDisplacementProbability(list<connectedPoint_t> &component)
{
	double tempProduct = 1.0;
	//Go through list of points, multiply their products, return
	for (list<connectedPoint_t>::iterator pointIter = component.begin(); pointIter != component.end(); pointIter++)
		tempProduct *= pixDispProbMap[pointIter->y*width + pointIter->x];
	return tempProduct;
}


//selectiveDifference
double NonParametric::getBGProb(NonParametricPixel *pixMap, unsigned int yCurPos, unsigned int xCurPos, unsigned int yBufPos, unsigned int xBufPos, Image8 *currentImage)
{
	double pixel[3];
	//Do color conversions here
	pixel[0] = currentImage->read(yCurPos, xCurPos, 0);
	pixel[1] = currentImage->read(yCurPos, xCurPos, 1);
	pixel[2] = currentImage->read(yCurPos, xCurPos, 2);
	return pixMap[yBufPos*width + xBufPos].getBGProb(pixel);
}

//The goal of this is to search a circular neighborhood an find the max pixel probability
double NonParametric::getNeighBgProb(NonParametricPixel *pixMap, unsigned int yPos, unsigned int xPos, Image8 *currentImage)
{
	//Iterate through a circular radius around a point
	//We start with a square, and then we decide if we are going to search a certain pixel if it is inside our virtual circular distance (rounded)
	
	int ceilRadius = (int)ceil(neighborhoodRadius);
	double maxPixelProbability = 0.0;
	//These doubles are to get around gcc's improper reporting of this as always true (unsigned int - {int/unsigned int} >= 0)
	unsigned int leftBound = ((double)xPos - ceilRadius) >= 0 ? xPos - ceilRadius : 0;
	unsigned int rightBound = xPos + ceilRadius  < currentImage->getWidth() ? xPos + ceilRadius :  currentImage->getWidth() - 1;
	unsigned int upperBound = (unsigned int)(((double)yPos - ceilRadius) >= 0 ? yPos - neighborhoodRadius : 0);
	unsigned int lowerBound = yPos + ceilRadius  < currentImage->getHeight() ? yPos + ceilRadius :  currentImage->getHeight() - 1;
	for (unsigned int yIter = upperBound; yIter <= lowerBound;yIter++)
		for(unsigned int xIter = leftBound; xIter <= rightBound;xIter++)
		{
			if (sqrt((yPos - yIter)*(yPos - yIter) + (xPos - xIter)*(xPos - xIter)) <= neighborhoodRadius)
			{
				double tempBgProb = getBGProb(pixMap, yPos, xPos, yIter, xIter, currentImage);
				if (tempBgProb > maxPixelProbability)
					maxPixelProbability = tempBgProb;
			}
		}
	return maxPixelProbability;
}

void NonParametric::subtract(Image8 &differenceImage, Image8 &imageRan)
{
	assert(differenceImage.getHeight() == height);
	assert(differenceImage.getWidth() == width);
	//TODO Do sanity checks that everything that should equal is

	differenceImage.zero();
	Image8 *currentImage = new Image8(height,width,channels);
	imProvide->getImage(*currentImage);
	imageRan.copyFrom(*currentImage);
	
	tempDiffImages[0]->zero();
	tempDiffImages[1]->zero();
	//NOTE <----------------------==== The below values have been commented so that it can just return a thresholded probability
	for (unsigned int pixMapIter = 0; pixMapIter < 2; pixMapIter++)
	{
		for (unsigned int yIter = 0; yIter < height; yIter++)
			for (unsigned int xIter = 0; xIter < width; xIter++)
			{
				double pixel[3];
				//Do color conversions here
				pixel[0] = currentImage->read(yIter, xIter, 0);
				pixel[1] = currentImage->read(yIter, xIter, 1);
				pixel[2] = currentImage->read(yIter, xIter, 2);
				//Note minBackgroundThreshold has been already multiplied by the normalization factor, meaning we don't have to normalize our probability
				//cout << pixMaps[pixMapIter][yIter*width + xIter].getBGProb(pixel) << endl;
				if (pixMaps[pixMapIter][yIter*width + xIter].getBGProbShort(pixel,minBackgroundThreshold))
				{	
					tempDiffImages[pixMapIter]->write(yIter,xIter,0,255);
				
					//Calculate a map that represents the pixel displacement probabilities
					//pixDispProbMap[yIter*width + xIter] = getNeighBgProb(pixMaps[pixMapIter],yIter, xIter, currentImage);
				}
				else
				{
					//(unset pixels should never be access, so they will be set to -1, thus causing the connected component disp prob to possibly be negative if these are access in error (just extra protection))
					pixDispProbMap[yIter*width + xIter] = -1.0;
				}	
			}
/*
		//Remove noise
		//We calculate the object displacement probability, if it is over our threshold
		list<list<connectedPoint_t> > objectList;
		getConnectedObjects((*tempDiffImages[pixMapIter]), objectList);
		for (list<list<connectedPoint_t> >::iterator objIter = objectList.begin(); objIter != objectList.end(); objIter++)
		{
			double ccDispProb = getCCDisplacementProbability(*objIter);
			//Now if this is greater than some constant value, if it is less than or equal to it, then it will stay foreground
			if (ccDispProb > ccDispThresh)//th_1 in paper
			{
				//For every pixel who's pixel displacement probability is greater than a threshold, we set it to be background
				for (list<connectedPoint_t>::iterator pointIter = objIter->begin(); pointIter != objIter->end(); pointIter++)
					if (pixDispProbMap[pointIter->y*width + pointIter->x] > dispThresh)//th_2 in paper
						tempDiffImages[pixMapIter]->write(pointIter->y,pointIter->x,0,0);
			}
		}
*/
	}

	for (unsigned int yIter = 0; yIter < height; yIter++)
		for (unsigned int xIter = 0; xIter < width; xIter++)
		{
			//TODO Also add pixels from 0 that are touching
			if (tempDiffImages[0]->read(yIter,xIter) == 255 &&  tempDiffImages[1]->read(yIter,xIter) == 255)
			{
				differenceImage.write(yIter,xIter,0,255);
			}
		}
	
	for (unsigned int yIter = 0; yIter < height; yIter++)
		for (unsigned int xIter = 0; xIter < width; xIter++)
		{
			double pixel[3];
			//Do color conversions here
			pixel[0] = currentImage->read(yIter, xIter, 0);
			pixel[1] = currentImage->read(yIter, xIter, 1);
			pixel[2] = currentImage->read(yIter, xIter, 2);
			
			if (differenceImage.read(yIter,xIter) == 255)
			{
				pixMaps[0][yIter*width + xIter].updateSample(pixel);
			}
			pixMaps[1][yIter*width + xIter].updateSample(pixel);
		}
		
		delete currentImage;
}
