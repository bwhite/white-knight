#include "fitness.h"

Fitness::Fitness(unsigned int height, unsigned int width, char* truthPath, char *framesString,std::list<vector<string> > &configList) : height(height), width(width)
{
	//Load config entries from file
	unsigned int notLoaded = 0;
	notLoaded += FindValue(configList, "TruthExtension", truthExtension);
	assert(!notLoaded);
	
	//Load "truth" frame numbers for autotuning //TODO Use a format such as NumTruthFrames 10, than look for 0-9 frames
	Devil jpeg;
	char truthFileName[100];
		
	char *framePoint = 0;
	do
	{		
		truthFrame_t tempTruthFrame;
		tempTruthFrame.image = new Image8(height,width,1);
		framePoint = strtok(framesString,"\\");

		if(framePoint == 0)
			break;
		
		framesString = 0;//So that strtok will give us more next time
		sscanf(framePoint,"%u",&tempTruthFrame.frame);
		sprintf(truthFileName, "%s/%.4u.%s", truthPath, tempTruthFrame.frame, truthExtension);
		jpeg.readImage(*(tempTruthFrame.image), truthFileName);

		//This fixes the error brought on by compression of the truth frames, so we say <= 30 is 0, >= 225 is 255, the rest is ignore
		for (unsigned int o = 0; o < height;o++)
			for (unsigned int p = 0; p < width; p++)
			{
				if (tempTruthFrame.image->read(o,p) >=225)
					tempTruthFrame.image->write(o,p,0,255);
				else if (tempTruthFrame.image->read(o,p) <= 30)
					tempTruthFrame.image->write(o,p,0,0);
			}
		frames.push_back(tempTruthFrame);
	} while(framePoint != 0);//Effectively while true for this guy
}

Fitness::~Fitness()
{
	for (list<truthFrame_t>::iterator truthIter = frames.begin(); truthIter != frames.end(); truthIter++)
	{
		delete truthIter->image;
	}
}

bool Fitness::hasTruthFrame(unsigned int frameNumber) const
{
	bool hasFrame = false;
	for (list<truthFrame_t>::const_iterator truthIter = frames.begin(); truthIter != frames.end(); truthIter++)
	{
		if (truthIter->frame == frameNumber)
		{
			hasFrame = true;
			break;
		}
	}
	return hasFrame;
}

Image8* Fitness::getTruthFrame(unsigned int frameNumber)
{
	Image8* tempImage = 0;
	for (list<truthFrame_t>::iterator truthIter = frames.begin(); truthIter != frames.end(); truthIter++)
	{
		if (truthIter->frame == frameNumber)
		{
			tempImage = truthIter->image;
			break;
		}
	}
	return tempImage;
}

///Returns the largest truthFrame number
unsigned int Fitness::getLastTruthFrame()
{
	unsigned int maxTruthFrame = 0;
	for (list<truthFrame_t>::iterator truthIter = frames.begin(); truthIter != frames.end(); truthIter++)
	{
		if (truthIter->frame > maxTruthFrame)
			maxTruthFrame = truthIter->frame;
	}
	return maxTruthFrame;
}

///This will take the given one channel input image, and compute the fscore for it based on a given truth image
double Fitness::computeFrameFitness(Image8 &difference, unsigned int frameNumber, FILE *file)
{
	assert(difference.getChannels() == 1);
	assert(hasTruthFrame(frameNumber));
	assert(difference.getHeight() == height);
	assert(difference.getWidth() == width);

	//Ensure the image is 0/255 binary by saying >=128 is positive
	/*for (unsigned int o = 0; o < height;o++)
		for (unsigned int p = 0; p < width; p++)
		{
			if (difference(o,p) >=128)
				difference(o,p) = 255;
			else
				difference(o,p) = 0;
		}*/
	
	unsigned int correctNegative = 0;
	unsigned int correctPositive = 0;
	unsigned int falsePositive = 0;
	unsigned int falseNegative = 0;

	const Image8 *truthImage = getTruthFrame(frameNumber);

	//The else if's make this have the property that if something is not 0 or 255, it is not scored
	for (unsigned int o = 0; o < height;o++)
		for (unsigned int p = 0; p < width; p++)
		{
			if (truthImage->read(o,p) == 0)
			{
				if (difference.read(o,p) == 0)
					correctNegative++;
				else if (difference.read(o,p) == 255)
					falsePositive++;
			}
			else if (truthImage->read(o,p) == 255)
			{
				if (difference.read(o,p) == 0)
					falseNegative++;
				else if (difference.read(o,p) == 255)
					correctPositive++;
			}
		}
		
	//This stops divide by zero's and the like from happening if we don't have any correct positives
	if (correctPositive == 0)
	{
		correctPositive = 1;
		cerr << "CorrectPositive is zero! (It is being set to one, but this shouldn't happen in general)" << endl;
	}
	
	double precision = (double)correctPositive / (double)(correctPositive + falsePositive);
	double sensitivity = (double)correctPositive / (double)(correctPositive + falseNegative);
	double fscore = 2.0 * precision * sensitivity / (precision + sensitivity);

	if (file)
		fprintf(file, "%u %u %u %u %f\n",correctNegative, correctPositive, falsePositive, falseNegative, fscore);
	return fscore;
}
