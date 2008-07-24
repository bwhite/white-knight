#ifndef NONPARAM_H
#define NONPARAM_H
#include <list>
#include <vector>
#include <cmath>
#include "../BackgroundSubtraction.h"
#include "../configTools/configReader.h"
#include "../ImageProvider.h"
#include "../imageTools/connectedComponents.h"

using namespace std;

//These should be removed and the bandwidth calculation should be fixed
extern double bandR;
extern double bandG;
extern double bandB;

/* Each pixel has 2 lists of double*'s that keep track of both image buffers
 */
class NonParametricPixel
{
private:
	list<double*> buffer;
	double* bandwidth;
	static unsigned int channels;
	static unsigned int publicSize;
	//Tunable Parameters
	unsigned int maxBufsize;
public:	
	//Required channels and config to have already been updated!
	NonParametricPixel()
	{
		maxBufsize = publicSize;
		bandwidth = new double[channels];
	}
	~NonParametricPixel()
	{
		while(!buffer.empty())
		{
			delete [] buffer.back();
			buffer.pop_back();
		}
		delete [] bandwidth;
	}
	
	//Used to initialize buffer
	inline void sampleInitialize(double* sample)
	{
		if (maxBufsize > buffer.size())
		{
			//cout << (sample)[0] << endl;
			double *tempSample = new double[channels];
			memcpy(tempSample, sample, sizeof(double) * channels);
			buffer.push_front(tempSample);
		}
	}
	
	//Used for bandwidth sharing
	inline double* getBandwidth()
	{
		return bandwidth;
	}
	
	inline void setBandwidth(double *newBand)
	{
		memcpy(bandwidth, newBand, sizeof(double) * channels);
	}

	inline void updateSample(double* sample)
	{
		double *tempSample = new double[channels];
		//cout << (sample)[0] << endl;
		memcpy(tempSample, sample, sizeof(double) * channels);
		buffer.push_front(tempSample);
	
		cleanBufferEnd();
	}
	
	//Use Elgammals median absolute difference method 
	//----NOTE should only be used on blind update method when pair aligned or consecutive in time
	//TODO Figure out if we should do this often, also we might want a lower and upper bound
	inline void calculateBandwidth()
	{//TODO This seems to have a bug in it!
		/*assert(buffer.size() > 1);//If too small, this could get ugly, shouldn't be called if its to small anyways
		//For now we will do this the slow way
		//A fast way to do this repeatedly is to keep a queue of all of the iterators, then sort a list (each queue iterator points to a list item)
		//Then we sort the list, to add a new element, we simply delete the iterator from the queue and pop it off, then we add the new element to the list, push its iterator, sort the list
		vector<double> *absoluteDiffVec = new vector<double>[channels];
		for (unsigned int chanIter = 0; chanIter < channels; chanIter++)
		{
			list<double*>::iterator sampleIter = buffer.begin();
			list<double*>::iterator sampleIterNext = buffer.begin();
			sampleIterNext++;
			while (sampleIterNext != buffer.end())
			{
				//cout << abs((*sampleIter)[chanIter]-(*sampleIterNext)[chanIter]) << endl;
				absoluteDiffVec[chanIter].push_back(abs((*sampleIter)[chanIter]-(*sampleIterNext)[chanIter]));
				sampleIter++;
				sampleIterNext++;
			}
			//sortlist
			sort(absoluteDiffVec[chanIter].begin(), absoluteDiffVec[chanIter].end());
			//setband
			unsigned int vecSize = absoluteDiffVec[chanIter].size();
			if (vecSize % 2 == 0)//if divisible by 2
			{
				bandwidth[chanIter] = (absoluteDiffVec[chanIter][vecSize/2 - 1] + absoluteDiffVec[chanIter][vecSize/2])/2.0;
			}
			else
			{
				bandwidth[chanIter] = absoluteDiffVec[chanIter][vecSize/2];
			}
			//cout << bandwidth[chanIter] << endl;
		}
		delete [] absoluteDiffVec;*/
		bandwidth[0] = bandR;
		bandwidth[1] = bandG;
		bandwidth[2] = bandB;
	}
	
	//Short circuits when threshold is reached, basically, this is correcting the statement "The Pixel is Foreground"
	inline bool getBGProbShort(double *pixel, double threshold)
	{
		double pixelProbability = 0.0;
		for (list<double*>::iterator sampleIter = buffer.begin(); sampleIter != buffer.end(); sampleIter++)
		{
			double tempProduct = 1.0;
			//Joint probability among all channels
			for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			{
				double delta = pixel[channelIter] - (*sampleIter)[channelIter];
				tempProduct *= exp(-.5*delta*delta/bandwidth[channelIter])/sqrt(2.0 * M_PI * bandwidth[channelIter]);
			}
			pixelProbability += tempProduct;
			if (pixelProbability >= threshold)
				return false;
		}
		return true;
	}
	
	inline double getBGProb(double *pixel)
	{
		double pixelProbability = 0.0;
		for (list<double*>::iterator sampleIter = buffer.begin(); sampleIter != buffer.end(); sampleIter++)
		{
			double tempProduct = 1.0;
			//Joint probability among all channels
			for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			{
				double delta = pixel[channelIter] - (*sampleIter)[channelIter];
				tempProduct *= exp(-.5*delta*delta/bandwidth[channelIter])/sqrt(2.0 * M_PI * bandwidth[channelIter]);
			}
			pixelProbability += tempProduct;
		}
		return pixelProbability;
	}
	
	//Check bounds, if necessary delete the oldest to fit size requirements
	inline void cleanBufferEnd()
	{
		bool updated;
		do
		{
			updated = false;
			if (maxBufsize < buffer.size())
			{
				updated = true;
				delete [] buffer.back();
				buffer.pop_back();
			}
		} while(updated);
	}
	
	//NOTE This should be set BEFORE initialization and NEVER changed!  C++ has weak initialization support for array new : (
	static void setChannels(unsigned int channels)
	{
		NonParametricPixel::channels = channels;
	}
	
	//NOTE This should be set BEFORE initialization!  Whatever value it is set to will be copied to its internal variable so that several different pixel maps can be created with different buffer sizes.
	//C++ has weak initialization support for array new : (
	static void setSize(unsigned int size)
	{
		NonParametricPixel::publicSize = size;
	}
};

/// This class is an implementation of a NonParametric Background Subtraction (Elgammal)
/// \author Brandyn White
class NonParametric : public BackgroundSubtraction
{
public:
	NonParametric(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, list<vector<string> > configList);/* Height, Width, config, image handler */
	~NonParametric();
	void subtract (Image8 &differenceImage, Image8 &imageRan);
	virtual void hint(const char * name, Image8 &hintImage)
	{
		cout << "HINT: " << name << "not supported!\n";
	}
	virtual void hint(const char * name, double hintValue)
	{
		cout << "HINT: " << name << "not supported!\n";
	}
	virtual void resetHints()
	{
	}
	virtual void getBackgroundImage(Image8 &backgroundImage)
	{
		cerr << "getBackgroundImage Not implemented in NonParametric!!!" << endl;
		exit(1);
	}
private:
	
	double getBGProb(NonParametricPixel *pixMap, unsigned int yCurPos, unsigned int xCurPos, unsigned int yBufPos, unsigned int xBufPos, Image8 *currentImage);
	double getNeighBgProb(NonParametricPixel *pixMap, unsigned int yPos, unsigned int xPos, Image8 *currentImage);
	double getCCDisplacementProbability(list<connectedPoint_t> &component);
	ImageProvider *imProvide;
	double *pixDispProbMap;
	NonParametricPixel *pixMaps[2];//0 is selective, 1 is nonselective
	Image8 *tempDiffImages[2];
	///Our queue of images which is initialized in the constructor.  We add one to the front and remove one from the back per run.
	list<Image8*> imageBuffer;
	vector<double> bandwidth;
	vector<double> denominator;
	const unsigned int height;
	const unsigned int width;
	const unsigned int channels;
	//Variables
	///Defines the number of frames we want to keep in our buffer
	unsigned int selectiveWindowSize;
	unsigned int nonselectiveWindowSize;
	double neighborhoodRadius;
	double dispThresh;
	double ccDispThresh;
	double minBackgroundThreshold;
};
#endif
