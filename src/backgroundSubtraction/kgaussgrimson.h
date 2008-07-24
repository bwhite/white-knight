/*
TODO
1.  Fix weight system (initial weight, weight thresh, etc)
2.  Create function to normalize weights, and run at appropriate times
3.  Create function to sort gaussians
	a.  Sort based on weight/sqrt(variance)
	b.  Find shortest mahalanobis distance
	c.  If less than threshold (2.5 std. dev or equivelantly (2.5)^2 var.)
	d.  Background if its one of the first 'B' distributions, where B equals (see B= eqn)
*/

#ifndef KGAUSSGRIM_H
#define KGAUSSGRIM_H
#include <math.h>
#include <list>
#include <cassert>
#include "../configTools/configReader.h"
#include "../ImageProvider.h"
#include "../BackgroundSubtraction.h"
using namespace std;

class GrimGaussian
{
protected:
	double weight;
	double weightStdDevRatio;
	double *mu;    //Mean
	double variance; //sigma^2
	double rho; //Learning rate for weighted average
	double alpha; //Learning rate for weights, each gaussian gets its own as we want to tweak it
	//Config variables
	static unsigned int channels;

public:
	GrimGaussian()
	{
		mu = new double[channels];
	}

	~GrimGaussian()
	{
		delete [] mu;
	}

	//NOTE This should be set BEFORE initialization and NEVER changed!  C++ has weak initialization support for array new : (
	static void setChannels(unsigned int channels)
	{
		GrimGaussian::channels = channels;
	}
	
	void printDistributionInfo()
	{
		cout << "Weight: " << weight <<  " Variance: " << variance << " Mu: ";
		for (unsigned int chanIter = 0; chanIter < channels; chanIter++)
			cout << " " << mu[chanIter];
		cout << endl;
	}
	
	//Ratio is w^2/sigma^2
	void computeRatio()
	{
		if (weight == 0.0)
			weightStdDevRatio = 0;
		else if (variance == 0.0)
			weightStdDevRatio = 10000;//TODO Change to be the max double value
		else
			weightStdDevRatio = (weight*weight)/variance;
	}
	
	//TODO Make this a lookup table, look into its correctness as well
	void computeRho(const unsigned char* pixel)
	{
		double nu;
		if (channels == 3)//NOTE This is an optimization
		{
			double powBase = 2*M_PI*variance;
			nu = exp(-.5 * getSumDifferenceSquared(pixel) / variance) / (powBase * sqrt(powBase) );
		}
		else
			nu = exp(-.5 * getSumDifferenceSquared(pixel) / variance) / (pow(2*M_PI*variance,(double)channels/2.0));
		rho = nu*alpha;
	}
	
	inline double getRatio()
	{
		return weightStdDevRatio;
	}

	static int loadConfig(std::list<vector<string> > configList)
	{
		return 0;
	}

	inline void setWeight(double weight)
	{
		this->weight = weight;
	}
	
	inline void setAlpha(double alpha)
	{
		this->alpha = alpha;
	}

	inline double getWeight()
	{
		return weight;
	}

	inline void incWeight()
	{
		weight += alpha;
	}

	inline void decWeight()
	{
		weight *= (1.0-alpha);
	}

	inline void setMu(const unsigned char* pixel)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] = pixel[channelIter];
	}
	
	inline void setMu(const double* pixel)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] = pixel[channelIter];
	}

	inline void muToImage(unsigned int yPos,unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			currentImage.write(yPos, xPos, channelIter, (unsigned char)round(mu[channelIter]));
	}

	inline void setVariance(double variance)
	{
		this->variance = variance;
	}

	inline void clear()
	{
		weight = 0;
	}

	//Used many times in the algorithm, takes the difference of the current pixel vector - the mean vector, and transpose multiplies that by itself
	inline double getSumDifferenceSquared(const unsigned char* pixel)
	{
		double sum = 0;

		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double difference = pixel[channelIter] - mu[channelIter];
			sum += (difference*difference);
		}
		return sum;
	}

	double calculateDistance(const unsigned char* pixel)
	{	
		return getSumDifferenceSquared(pixel) / (variance);//Basically mahalanobis distance squared and normalized for number of channels (to allow for the same 2.5 value to be used for any number of channels), this means we don't take the root of it which is compared to a thresh anyways (aka no accuracy change)
	}

	void calculateVariance(const unsigned char* pixel)
	{
		variance = (1 - rho) * variance + rho * getSumDifferenceSquared(pixel);
	}

	void calculateMean(const unsigned char* pixel)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] =  (1.0 - rho) * mu[channelIter] + rho * pixel[channelIter]; //This is a weighted average
	}
};

//Initialize static member variables

unsigned int GrimGaussian::channels;//NOTE This is a hack around c++'s limitation on class inititalization with array new

class KGaussianGrimson : public BackgroundSubtraction
{
private:
	const unsigned int height;
	const unsigned int width;
	const unsigned int channels;
	Image8 currentImage;
	ImageProvider *imProvide;

	/***********Background Settings***********/
	unsigned int numGaussians;
	double initialWeight;
	double initialVariance;
	double minTotalBgWeight;
	unsigned int savedGaussIndex; //This is an optimization that also makes the code cleaner, we use this to save the last i/j index used
	
	//Here are the specialized paramaters for hinting
	double alphaIllumination; //Learning rate for weights during illumination changes
	double alphaGeneral; //Learning rate for weights during normal conditions
	double alphaForced; //Learning rate for weights during normal conditions
	double alphaForeground; //Learning rate for weights when known object is covering the background

	/******************Thresholds********************/
	double minDistanceThreshold;
	double minDistanceThresholdIllumination;
	
	GrimGaussian **gauss_data;//Where the gaussian map is held
	double *mahalanobis_distance;
	
	/*****************HINTS***************************/
	bool globalIlluminationChange;

	inline GrimGaussian& gauss(unsigned int height, unsigned int width, unsigned int k)
	{
		assert(height < this->height);
		assert(width < this->width);
		assert(k < this->numGaussians);//TODO Make a seperate assert type for these
		return *gauss_data[numGaussians * (height * this->width + width) + k];
	}
	
	void normalizeWeights(unsigned int height, unsigned int width)
	{
		double sum = 0;
		for (unsigned int weightIter = 0; weightIter < numGaussians; weightIter++)
		{
			sum += gauss(height,width, weightIter).getWeight();
		}
		
		if (sum == 0)
			sum = .000001;//TODO Put smallest nonzero number here
		for (unsigned int normIter = 0; normIter < numGaussians; normIter++)
			gauss(height,width, normIter).setWeight(gauss(height,width, normIter).getWeight() / sum);
	}
	
	//TODO Only sort matched distributions
	void sortDistributions(unsigned int height, unsigned int width)
	{
		//The number of distributions is generally between 3-5, and between frames this should rarely change
		//A trivial bubble sort will be used because of this
		//The sorting is based on each distributions weight/sqrt(variance) descending

		//Compute sort ratio
		for (unsigned int ratioIter = 0; ratioIter < numGaussians; ratioIter++)
			gauss(height,width, ratioIter).computeRatio();

		unsigned int shiftConstant = numGaussians * (height * this->width + width);
		
		for (unsigned int i = 0; i < numGaussians; i++)
		{
			bool isSorted = true;
			for (unsigned int j = 0; j < numGaussians - 1; j++)
			{		
				if (gauss(height,width, j).getRatio() < gauss(height,width, j+1).getRatio())//Descending
				{
					//Swap
					isSorted = false;
					GrimGaussian *tempGauss = gauss_data[shiftConstant + j];
					gauss_data[shiftConstant + j] = gauss_data[shiftConstant + j + 1];
					gauss_data[shiftConstant + j + 1] = tempGauss;
				}
			}
			if (isSorted)
				break;
		}
		/*cout << "Sort[";
		for (unsigned int sortIter = 0; sortIter < numGaussians; sortIter++)
			cout << (gauss_data[shiftConstant + sortIter])->getRatio() << " ";
		cout << "]" << endl;
		*/
	}
	
	unsigned int minIndexSum(unsigned int height, unsigned int width)
	{	
		double weightTotalSum = 0;
		
		//Find the total weight to allow for normalization
		for (unsigned int weightTotalIter = 0; weightTotalIter < numGaussians; weightTotalIter++)
		{
			weightTotalSum += gauss(height,width, weightTotalIter).getWeight();
		}
		
		double tempSumThresh = minTotalBgWeight * weightTotalSum;//Optimization
		//Find the lowest number of gaussians that allows us to meet our weight threshold for amount of background modeled
		double tempSum = 0;
		for (unsigned int minIndexIter = 0; minIndexIter < numGaussians; minIndexIter++)
		{
			tempSum += gauss(height,width, minIndexIter).getWeight();
			if (tempSum > tempSumThresh)
				return minIndexIter;
		}

		assert(0);//We should never be here as it would mean that the sum of the weights are less than the minTotalBgWeight, which can't happen
		return numGaussians - 1;
	}

	inline void saveGaussIndex(unsigned int height, unsigned int width)
	{
		assert(height < this->height);
		assert(width < this->width);
		savedGaussIndex = numGaussians * (height * this->width + width);
	}
	
	inline void printPixel(unsigned int height, unsigned int width)
	{
			for (unsigned int k = 0; k < numGaussians; k++)
			{
				gauss(height,width, k).printDistributionInfo();
			}
	}
	
	virtual void getBackgroundImage(Image8 &backgroundImage)
	{
		assert(backgroundImage.getChannels() == channels);
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				gauss(i,j,0).muToImage(i,j,backgroundImage);
			}
		}
	}

	//This is an optimization that also makes the code cleaner, we use this to save the last i/j index used, care must be taken to ensure that the lastGaussIndex is correct!
	inline GrimGaussian& gauss(unsigned int k)
	{
		assert(k < this->numGaussians);
		return *gauss_data[savedGaussIndex + k];
	}

public:
	virtual void hint(const char * name, Image8 &hintImage)
	{
		if (!strcmp(name,"ForceBackground"))
		{
			for (unsigned int i = 0; i < height; i++)
			{
				for (unsigned int j = 0; j < width; j++)
				{
					if (hintImage.read(i,j) > 0)
						for (unsigned int k = 0; k < this->numGaussians; k++)
							gauss(i,j,k).setAlpha(alphaForced);
				}
			}
		}
		else if (!strcmp(name,"ActiveObjects"))
		{
			for (unsigned int i = 0; i < height; i++)
			{
				for (unsigned int j = 0; j < width; j++)
				{
					if (hintImage.read(i,j) > 0)
						for (unsigned int k = 0; k < this->numGaussians; k++)
							gauss(i,j,k).setAlpha(alphaForeground);
				}
			}
		}
		else
			cout << "HINT: " << name << "not supported!\n";
	}
	
	virtual void hint(const char * name, double hintValue)
	{
		if (!strcmp(name,"GlobalIlluminationChange"))
		{
			if (hintValue)
			{
				cout << "Global Illumination Change" << endl;
				globalIlluminationChange = true;
				for (unsigned int i = 0; i < height; i++)
				{
					for (unsigned int j = 0; j < width; j++)
					{
						for (unsigned int k = 0; k < this->numGaussians; k++)
							gauss(i,j,k).setAlpha(alphaIllumination);
					}
				}
			}
		}
		else
			cout << "HINT: " << name << "not supported!\n";
	}
	
	virtual void resetHints()
	{
		globalIlluminationChange = false;
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				for (unsigned int k = 0; k < this->numGaussians; k++)
					gauss(i,j,k).setAlpha(alphaGeneral);
			}
		}
	}
	
	KGaussianGrimson(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, std::list<vector<string> > configList): height(height), width(width), channels(channels), currentImage(height,width,channels), globalIlluminationChange(false)
	{
		this->imProvide = imProvide;
		loadConfig(configList);//Load all of the settings from the config file, thus replacing anything above that is listed there
		GrimGaussian::setChannels(channels);
		gauss_data = new GrimGaussian*[numGaussians * height * width];
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			gauss_data[memIter] = new GrimGaussian;
		mahalanobis_distance = new double[numGaussians];

		imProvide->getImage(currentImage);
		assert(numGaussians > 1);
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian
				//Gaussian 0 is our "special" background model, as it takes the value directly from the image when initialized
				gauss(0).setMu(currentImage.getPixelPointer(i,j));
				gauss(0).setVariance(initialVariance);
				gauss(0).setWeight(initialWeight);
				gauss(0).setAlpha(alphaForced);
				for (unsigned int k = 1; k < numGaussians; k++)
				{
					gauss(k).setVariance(initialVariance);
					gauss(k).setWeight(0);
					gauss(k).setAlpha(alphaForced);
				}
				normalizeWeights(i,j);
			}
		}
		imProvide->reset();
	}

	~KGaussianGrimson()
	{
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			delete gauss_data[memIter];
		delete [] mahalanobis_distance;
		delete [] gauss_data;
	}

	void loadConfig(std::list<vector<string> > configList)
	{
		double tempMinDist;
		int notLoaded = 0;
		
		notLoaded += FindValue(configList, "NumberOfGaussians", numGaussians);
		notLoaded += FindValue(configList, "InitialWeight", initialWeight);
		notLoaded += FindValue(configList, "InitialVariance", initialVariance);
		notLoaded += FindValue(configList, "MinTotalBgWeight", minTotalBgWeight);
		
		//Different lambda values
		notLoaded += FindValue(configList, "MinDistanceThresh", tempMinDist);
		minDistanceThreshold = tempMinDist * tempMinDist;
		notLoaded += FindValue(configList, "MinDistanceThreshIllumination", tempMinDist);
		minDistanceThresholdIllumination = tempMinDist * tempMinDist;
		
		//Different alpha values
		notLoaded += FindValue(configList, "Alpha", alphaGeneral);
		notLoaded += FindValue(configList, "AlphaIllumination", alphaIllumination);
		notLoaded += FindValue(configList, "AlphaForced", alphaForced);
		notLoaded += FindValue(configList, "AlphaForeground", alphaForeground);
		
		notLoaded += GrimGaussian::loadConfig(configList);
		if (notLoaded != 0)
			exit(0);
	
	}

	/* This is the main kgaussian background subtraction.
	 * CurrentImage should be initialized with the current frame in the sequence
	 * All the other images will be cleared or filled as they need to be
	 * differenceImage is the main useful image from this, black is bg and white is fg
	 * backgroundImage shows the minMahalanobisDistance (aka the matching gaussian for each pixel) mu for each pixel, thus creating an image similar to the currentImage, but with less of the foreground object (ghosted)
	 */
	void subtract(Image8 &differenceImage, Image8 &imageRan)
	{
		differenceImage.zero();
		imProvide->getImage(currentImage);

		double currentMinDistanceThreshold;
		
		if (globalIlluminationChange)
		{
			currentMinDistanceThreshold = minDistanceThresholdIllumination;
		}
		else
			currentMinDistanceThreshold = minDistanceThreshold;

		for (unsigned int i = 0; i < height;i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				unsigned int distribution_matched_index = 0;
				bool matchedDist = false;
				bool fgPixel = true;
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian
				
				unsigned int weightIndThresh = minIndexSum(i,j);

				//Clear mahalanobis distance and reset weights that have gotten too small
				for (unsigned int k = 0; k < numGaussians; k++)
				{
					if (gauss(k).getWeight() > 0)
					{
						gauss(k).decWeight();// Decrement the weight, keep in mind this is before we add alpha to the matching gaussian
					}
					mahalanobis_distance[k] = -1;//We set the mahalanobis distance to -1 so that when outputted, we can tell that it wasn't calculated	
				}

				//Calculate the mahalanobis distance for each gaussian to the current pixel, find minimum mahalanobis distance, degenerate weights
				for (unsigned int k = 0; k < numGaussians; k++)
				{	
					if (gauss(k).getWeight() > 0)
					{
						mahalanobis_distance[k] = gauss(k).calculateDistance(currentImage.getPixelPointer(i,j));
						
						
						/* This is the key part of the algorithm, here we are saying that if the pixel is within a certain number
						 * of std. dev. away from distribution k, then we say we have found a match, now if this k is less or equal to
						 * our weightIndThresh (B), then this pixel is part of our background
						 */	
						if (mahalanobis_distance[k] < currentMinDistanceThreshold)
						{
							matchedDist = true;
							distribution_matched_index = k;
							if (k <= weightIndThresh)
								fgPixel = false;
							break;
						}
					}	
				}
				
				if (matchedDist && mahalanobis_distance[distribution_matched_index] < currentMinDistanceThreshold)
				{
					gauss(distribution_matched_index).computeRho(currentImage.getPixelPointer(i,j));
					gauss(distribution_matched_index).calculateMean(currentImage.getPixelPointer(i,j));
					gauss(distribution_matched_index).calculateVariance(currentImage.getPixelPointer(i,j));
					gauss(distribution_matched_index).incWeight();
				}
				else
				{
					unsigned int min_weight_index = numGaussians - 1;
					gauss(min_weight_index).setMu(currentImage.getPixelPointer(i,j));
					gauss(min_weight_index).setWeight(initialWeight);
					gauss(min_weight_index).setVariance(initialVariance);
				}
				//Since the normalize weights function is generally slow, we want to avoid unnecessary calls to it
				//So, if the matched distribution is the top distribution, and if its the only one that is BG, then we skip this
				//if (distribution_matched_index != 0 || weightIndThresh != 0)
					normalizeWeights(i,j);
				
				//We can skip the sort if our matched dist. is the first NOTE That this isn't exactly true, if the variance grows faster than weight declines
				if (distribution_matched_index != 0)
					sortDistributions(i,j);
					
				if (fgPixel)
					differenceImage.write(i,j,0,255);
				/*if (i == 100 && j == 243)
				{
					cout << "Pix:";
					for (unsigned int charIt = 0; charIt < channels; charIt++)
						cout << " " << (int)currentImage(i,j,charIt);
					for (unsigned int charIt = 0; charIt < numGaussians; charIt++)
						cout << " " << mahalanobis_distance[charIt];
					if (differenceImage(i,j) == 255)
						cout << " FG";
					cout << endl;
					printPixel(i,j);
				}*/
			}
		}
		imageRan.copyFrom(currentImage);
		resetHints();
	}
};
#endif
