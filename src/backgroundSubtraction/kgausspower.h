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

#ifndef KGAUSSPOWER_H
#define KGAUSSPOWER_H
#include <math.h>
#include <list>
#include <cassert>
#include "../BackgroundSubtraction.h"
using namespace std;

class PowerGaussian
{
protected:
	double weight;
	double weightStdDevRatio;
	double *mu;    //Mean
	double *variance; //sigma^2
	double rho; //Learning rate for weighted average (derived from alpha*beta)
	
	//Config variables
	static double alpha;//Computed learning rate for each distribution, if sufficiently small, we use alphaConst
	static unsigned int time;//Used in calculation of alpha
	static unsigned int channels;
	static bool isAlphaConst;//When 1/time <= our constant alpha value, we will flag this and stop keeping track of time (stops overflow too)
	static double alphaConst; //Learning rate for weights

public:
	PowerGaussian()
	{
		mu = new double[channels];
		variance = new double[channels];
	}

	~PowerGaussian()
	{
		delete [] mu;
		delete [] variance;
	}

	//NOTE This should be set BEFORE initialization and NEVER changed!  C++ has weak initialization support for array new : (
	static void setChannels(unsigned int channels)
	{
		PowerGaussian::channels = channels;
	}
	
	void printDistributionInfo()
	{
		cout << "Weight: " << weight <<  " Variance: ";
		for (unsigned int chanVarIter = 0; chanVarIter < channels; chanVarIter++)
			cout << " " << variance[chanVarIter];
		cout << " Mu: ";
		for (unsigned int chanMuIter = 0; chanMuIter < channels; chanMuIter++)
			cout << " " << mu[chanMuIter];
		cout << endl;
	}
	
	double getVarianceMagnitude()
	{
		double varianceSumOfSquares = 0.0;
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			varianceSumOfSquares += variance[channelIter] * variance[channelIter];
		return sqrt(varianceSumOfSquares);
	}
	
	//Ratio is w^2/sigma^2
	void computeRatio()
	{
		double variance = getVarianceMagnitude();
		if (weight == 0.0)
			weightStdDevRatio = 0;
		else if (variance == 0.0)
			weightStdDevRatio = 10000;//TODO Change to be the max double value
		else
			weightStdDevRatio = (weight*weight)/variance;
	}
	
	void computeRho()
	{
		rho = alpha / weight;
		//cout << "Rho: " << rho << " Alpha: " << alpha << " Weight: " << weight << endl;
	}
	
	inline double getRatio()
	{
		return weightStdDevRatio;
	}

	static int loadConfig(std::list<vector<string> > configList)
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "Alpha", alphaConst);
		return notLoaded;
	}

	inline void setWeight(double weight)
	{
		this->weight = weight;
	}

	inline double getWeight()
	{
		return weight;
	}

	//Used in keeping track of the number of frames witnessed, used for alpha value calculation
	static inline void incTime()
	{
		if (!isAlphaConst)
			time++;
	}

	static inline void computeAlpha()
	{
		alpha = 1.0/time;
		if (alpha < alphaConst)
		{
			isAlphaConst = true;
			alpha = alphaConst;
		}
	}
	
	static inline void resetTime()
	{
		isAlphaConst = false;
		time = 1;
	}

	inline void incWeight()
	{
		weight += alpha;
	}

	inline void decWeight()
	{
		weight *= (1.0-alpha);
	}

	inline void setMu(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] = currentImage.read(yPos, xPos, channelIter);
	}

	inline void muToImage(unsigned int yPos,unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			currentImage.write(yPos, xPos, channelIter,(unsigned char)round(mu[channelIter]));
	}

	inline void setVariance(double variance)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			this->variance[channelIter] = variance;
		}
	}

	inline void clear()
	{
		weight = 0;
	}

	//Used many times in the algorithm, takes the difference of the current pixel vector - the mean vector, and transpose multiplies that by itself
	inline double getSumDifferenceSquared(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		double sum = 0;
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double difference = currentImage.read(yPos, xPos, channelIter) - mu[channelIter];
			sum += (difference*difference);
		}
		return sum;
	}

	//Returns a squared distance (to reduce computational requirements, this is generally thresholded by a constant, so just square your constant)
	inline double calculateDistance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		//Do the sum of each square distance divided by its dimensions variance 
		double sum = 0;
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double difference = currentImage.read(yPos, xPos, channelIter) - mu[channelIter];
			sum += (difference*difference)/variance[channelIter];
		}
		return sum;//Basically mahalanobis distance squared, this means we don't take the root of it which is compared to a thresh anyways (aka no accuracy change)
	}

	void calculateVariance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double difference = currentImage.read(yPos, xPos, channelIter) - mu[channelIter];
			variance[channelIter] = (1.0 - rho) * variance[channelIter] + rho * difference * difference;
		}
	}

	inline void calculateMean(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] =  (1.0 - rho) * mu[channelIter] + rho * currentImage.read(yPos,xPos,channelIter); //This is a weighted average
	}
};

//Initialize static member variables
double PowerGaussian::alphaConst; //Constant learning rate for weights
double PowerGaussian::alpha;
unsigned int PowerGaussian::channels;//NOTE This is a hack around c++'s limitation on class inititalization with array new
unsigned int PowerGaussian::time = 1;//Used in calculation of alpha
bool PowerGaussian::isAlphaConst = false;

class KGaussianPower : public BackgroundSubtraction
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

	/******************Thresholds********************/
	double minDistanceThreshold;

	PowerGaussian **gauss_data;//Where the gaussian map is held
	double *mahalanobis_distance;

	inline PowerGaussian& gauss(unsigned int height, unsigned int width, unsigned int k)
	{
		assert(height < this->height);
		assert(width < this->width);
		assert(k < this->numGaussians);//TODO Make a seperate assert type for these
		return *gauss_data[numGaussians * (height * this->width + width) + k];
	}
	
	inline void normalizeWeights(unsigned int height, unsigned int width)
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
	
	inline void sortDistributions(unsigned int height, unsigned int width)
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
					PowerGaussian *tempGauss = gauss_data[shiftConstant + j];
					gauss_data[shiftConstant + j] = gauss_data[shiftConstant + j + 1];
					gauss_data[shiftConstant + j + 1] = tempGauss;
				}
			}
			if (isSorted)
				break;
		}
		/*
		//Useful to ensure proper sorting, uncomment to check
		cout << "Sort[";
		for (unsigned int sortIter = 0; sortIter < numGaussians; sortIter++)
			cout << (gauss_data[shiftConstant + sortIter])->getRatio() << " ";
		cout << "]" << endl;
		*/
	}
	
	inline unsigned int minIndexSum(unsigned int height, unsigned int width)
	{
		double tempSum = 0;
		
		double weightTotalSum = 0;
		
		//Find the total weight to allow for normalization
		for (unsigned int weightTotalIter = 0; weightTotalIter < numGaussians; weightTotalIter++)
		{
			weightTotalSum += gauss(height,width, weightTotalIter).getWeight();
		}
		
		//Find the lowest number of gaussians that allows us to meet our weight threshold for amount of background modeled
		for (unsigned int minIndexIter = 0; minIndexIter < numGaussians; minIndexIter++)
		{
			tempSum += gauss(height,width, minIndexIter).getWeight();
			if (tempSum / weightTotalSum > minTotalBgWeight)
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

	//This is an optimization that also makes the code cleaner, we use this to save the last i/j index used, care must be taken to ensure that the lastGaussIndex is correct!
	inline PowerGaussian& gauss(unsigned int k)
	{
		assert(k < this->numGaussians);
		return *gauss_data[savedGaussIndex + k];
	}

public:
	KGaussianPower(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, std::list<vector<string> > configList): height(height), width(width), channels(channels), currentImage(height,width,channels)
	{
		cout << "KguassPower" << endl;
		this->imProvide = imProvide;
		loadConfig(configList);//Load all of the settings from the config file, thus replacing anything above that is listed there
		PowerGaussian::setChannels(channels);
		gauss_data = new PowerGaussian*[numGaussians * height * width];
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			gauss_data[memIter] = new PowerGaussian;
		mahalanobis_distance = new double[numGaussians];

		imProvide->getImage(currentImage);
		imProvide->reset();
		assert(numGaussians > 1);
		
		PowerGaussian::resetTime();
		
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian
				//Gaussian 0 is our "special" background model, as it takes the value directly from the image when initialized
				gauss(0).clear();
				gauss(0).setMu(i, j, currentImage);
				gauss(0).setVariance(initialVariance);
				gauss(0).setWeight(initialWeight);
				for (unsigned int k = 1; k < numGaussians; k++)
				{
					gauss(k).clear();
					gauss(k).setVariance(initialVariance);
				}
				normalizeWeights(i,j);
			}
		}
	}

	~KGaussianPower()
	{
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			delete gauss_data[memIter];
		delete [] mahalanobis_distance;
		delete [] gauss_data;
	}

	void loadConfig(std::list<vector<string> > configList)
	{
		int notLoaded = 0;
		double tempMinDistanceThreshold;
		notLoaded += FindValue(configList, "NumberOfGaussians", numGaussians);
		notLoaded += FindValue(configList, "InitialWeight", initialWeight);
		notLoaded += FindValue(configList, "InitialVariance", initialVariance);
		notLoaded += FindValue(configList, "MinDistanceThresh", tempMinDistanceThreshold);
		notLoaded += FindValue(configList, "MinTotalBgWeight", minTotalBgWeight);
		notLoaded += PowerGaussian::loadConfig(configList);
		if (notLoaded != 0)
			exit(0);
			
		minDistanceThreshold = tempMinDistanceThreshold * tempMinDistanceThreshold;//We use a squared term for this for performance, so here we correct that
	}
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

		for (unsigned int i = 0; i < height;i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				unsigned int distribution_matched_index = 0;
				bool matchedDist = false;
				bool fgPixel = true;
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian
				
				unsigned int weightIndThresh = minIndexSum(i,j);
				PowerGaussian::computeAlpha();
				
				//Clear mahalanobis distance and reset weights that have gotten too small
				for (unsigned int k = 0; k < numGaussians; k++)
				{
					if (gauss(k).getWeight() > 0)
					{
							gauss(k).decWeight();// Degenerate the weight, keep in mind this is before we add alpha to the matching gaussian
					}
					mahalanobis_distance[k] = -1;//We set the mahalanobis distance to -1 so that when outputted, we can tell that it wasn't calculated	
				}

				//Calculate the mahalanobis distance for each gaussian to the current pixel, find minimum mahalanobis distance, degenerate weights
				for (unsigned int k = 0; k < numGaussians; k++)
				{	
					if (gauss(k).getWeight() > 0)
					{
						mahalanobis_distance[k] = gauss(k).calculateDistance(i,j,currentImage);
							
						if (mahalanobis_distance[k] < minDistanceThreshold)
						{
							matchedDist = true;
							distribution_matched_index = k;
							if (k <= weightIndThresh)
								fgPixel = false;
							break;
						}
					}	
				}

				if (matchedDist && mahalanobis_distance[distribution_matched_index] < minDistanceThreshold)
				{
					gauss(distribution_matched_index).incWeight();
					gauss(distribution_matched_index).computeAlpha();
					gauss(distribution_matched_index).computeRho();
					gauss(distribution_matched_index).calculateMean(i,j,currentImage);
					gauss(distribution_matched_index).calculateVariance(i,j,currentImage);
				}
				else
				{
					unsigned int min_weight_index = numGaussians - 1;
					gauss(min_weight_index).clear();
					gauss(min_weight_index).setMu(i,j,currentImage);
					gauss(min_weight_index).setWeight(initialWeight);
					gauss(min_weight_index).setVariance(initialVariance);
				}
				
				normalizeWeights(i,j);
				sortDistributions(i,j);
				
				if (fgPixel)
					differenceImage.write(i,j,0,255);
				/*if (i == 342 && j == 583)
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
		PowerGaussian::incTime();
		imageRan.copyFrom(currentImage);
	}
};
#endif
