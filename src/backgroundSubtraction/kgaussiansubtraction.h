#ifndef KGAUSS_H
#define KGAUSS_H
#include <math.h>
#include <list>
#include <cassert>
#include "../BackgroundSubtraction.h"
using namespace std;

class Gaussian
{
protected:
	double weight;

	unsigned short int frames;
	double *mu;    //Mean
	double *variance; //sigma^2

	//Config variables
	static unsigned int channels;
	static double alphaOn; //Learning rate for weights
	static double alphaOff; //Learning rate for weights
	static double rhoMean; //Learning rate for weighted average
	static double maxVarIncrease;
	static double maxVariance;
	static double minVariance;
	static double rhoVariance; //Used when we calculate variance with a fixed rhoVariance as opposed to using it as a function of the frames a gaussian has seen, this method is less correct, but more stable (same as mean)
	//To signify which you would like to use, set rhoVariance to negative if you'd like to calculate it based on frames as < 0 is functionally useless for everything else
	static double minWeight;

public:
	Gaussian()
	{
		mu = new double[channels];
		variance = new double[channels];
	}

	~Gaussian()
	{
		delete [] mu;
		delete [] variance;
	}

	//NOTE This should be set BEFORE initialization and NEVER changed!  C++ has weak initialization support for array new : (
	static void setChannels(unsigned int channels)
	{
		Gaussian::channels = channels;
	}

	static int loadConfig(std::list<vector<string> > configList)
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "AlphaOn", alphaOn);
		notLoaded += FindValue(configList, "AlphaOff", alphaOff);
		notLoaded += FindValue(configList, "RhoMean", rhoMean);
		notLoaded += FindValue(configList, "RhoVariance", rhoVariance);
		notLoaded += FindValue(configList, "MaxVariance", maxVariance);
		notLoaded += FindValue(configList, "MinVariance", minVariance);
		notLoaded += FindValue(configList, "MinWeight", minWeight);
		notLoaded += FindValue(configList, "MaxVarianceIncrease", maxVarIncrease);
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

	inline void incWeight()
	{
		weight += alphaOn + alphaOff;
	}

	inline void decWeight()
	{
		weight -= alphaOff;
	}

	inline void setFrames(int frames)
	{
		this->frames = frames;
	}

	inline unsigned short int getFrames()
	{
		return frames;
	}

	inline void incFrames()
	{
		frames++;
	}

	inline void setMu(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] = currentImage(yPos, xPos, channelIter);
	}

	inline void muToImage(unsigned int yPos,unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			currentImage(yPos, xPos, channelIter) = (unsigned char)round(mu[channelIter]);
	}

	inline void setVariance(double variance)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			this->variance[channelIter] = variance;
	}

	inline void clear()
	{
		weight = minWeight;
		frames = 0;
	}

	inline double calculateDistance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		double tempMahal = 0.0;
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double distance = currentImage(yPos, xPos, channelIter) - mu[channelIter];
			tempMahal += (distance*distance) / variance[channelIter];//Basically mahalanobis distance squared, this means we don't take the root of it which is compared to a thresh anyways (aka no accuracy change)
		}
		return tempMahal;
	}

	void calculateVariance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			double variance_delta = currentImage(yPos, xPos, channelIter) - mu[channelIter];

			double temp_variance;

			temp_variance = (1-rhoVariance) * variance[channelIter] + (variance_delta * variance_delta)*rhoVariance;

			if (temp_variance - variance[channelIter] > maxVarIncrease)
				variance[channelIter] += maxVarIncrease;
			else
				variance[channelIter] = temp_variance;

			if (variance[channelIter] > maxVariance)
			{
				variance[channelIter] = maxVariance;
			}

			if (variance[channelIter] < minVariance)
			{
				variance[channelIter] = minVariance;
			}
		}
	}

	inline void calculateMean(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] =  (1 - rhoMean) * mu[channelIter] + rhoMean * currentImage(yPos,xPos,channelIter); //This is a weighted average
	}
};

//Initialize static member variables
double Gaussian::alphaOn; //Learning rate for weights
double Gaussian::alphaOff; //Learning rate for weights
double Gaussian::rhoMean; //Learning rate for weighted average
double Gaussian::rhoVariance; //Learning rate for weighted average
double Gaussian::maxVarIncrease;
double Gaussian::maxVariance;
double Gaussian::minVariance;
double Gaussian::minWeight;
unsigned int Gaussian::channels;//NOTE This is a hack around c++'s limitation on class inititalization with array new

class KGaussianSubtraction : public BackgroundSubtraction
{
private:

	const unsigned int height;
	const unsigned int width;
	const unsigned int channels;
	Image8 currentImage;
	ImageProvider *imProvide;

	/***********Background Settings***********/
	unsigned int numGaussians;
	double alphaOn; //Learning rate for weights
	double alphaOff; //Learning rate for weights
	double minWeight;
	double maxWeight;
	double initialWeight;
	double initialBgWeight;
	double initialVariance;
	unsigned int savedGaussIndex; //This is an optimization that also makes the code cleaner, we use this to save the last i/j index used

	/******************Thresholds********************/
	double minDistanceThreshold;
	double minDistanceHighPass;
	double weightThreshold;

	Gaussian *gauss_data;//Where the gaussian map is held
	double *mahalanobis_distance;

	inline Gaussian& gauss(unsigned int height, unsigned int width, unsigned int k)
	{
		assert(height < this->height);
		assert(width < this->width);
		assert(k < this->numGaussians);
		return gauss_data[numGaussians * (height * this->width + width) + k];
	}

	inline void saveGaussIndex(unsigned int height, unsigned int width)
	{
		assert(height < this->height);
		assert(width < this->width);
		savedGaussIndex = numGaussians * (height * this->width + width);
	}

	//This is an optimization that also makes the code cleaner, we use this to save the last i/j index used, care must be taken to ensure that the lastGaussIndex is correct!
	inline Gaussian& gauss(unsigned int k)
	{
		assert(k < this->numGaussians);
		return gauss_data[savedGaussIndex + k];
	}

public:
	KGaussianSubtraction(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, std::list<vector<string> > configList): height(height), width(width), channels(channels), currentImage(height,width,channels)
	{
		this->imProvide = imProvide;
		loadConfig(configList);//Load all of the settings from the config file, thus replacing anything above that is listed there
		Gaussian::setChannels(channels);
		gauss_data = new Gaussian[numGaussians * height * width];
		mahalanobis_distance = new double[numGaussians];

		imProvide->getImage(currentImage);
		imProvide->reset();
		assert(numGaussians > 1);
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
			{
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian
				//Gaussian 0 is our "special" background model, as it takes the value directly from the image when initialized
				gauss(0).setMu(i, j, currentImage);
				gauss(0).setVariance(initialVariance);
				gauss(0).setWeight(initialBgWeight);
				gauss(0).setFrames(1);
				for (unsigned int k = 1; k < numGaussians; k++)
				{
					gauss(k).setWeight(minWeight);
					gauss(k).setFrames(0);
				}
			}
		}
	}

	~KGaussianSubtraction()
	{
		delete [] mahalanobis_distance;
		delete [] gauss_data;
	}

	void loadConfig(std::list<vector<string> > configList)
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "NumberOfGaussians", numGaussians);
		notLoaded += FindValue(configList, "MinWeight", minWeight);
		notLoaded += FindValue(configList, "MaxWeight", maxWeight);
		notLoaded += FindValue(configList, "InitialBgWeight", initialBgWeight);
		notLoaded += FindValue(configList, "InitialWeight", initialWeight);
		notLoaded += FindValue(configList, "InitialVariance", initialVariance);
		notLoaded += FindValue(configList, "MinSqrMahalDistanceThresh", minDistanceThreshold);
		notLoaded += FindValue(configList, "MinSqrMahalDistanceHighPass", minDistanceHighPass);
		notLoaded += FindValue(configList, "WeightThresh", weightThreshold);
		notLoaded += Gaussian::loadConfig(configList);
		if (notLoaded != 0)
			exit(0);

		minDistanceHighPass += minDistanceThreshold;//NOTE Due to these additions the parameters they represent are relative
		initialBgWeight += weightThreshold;
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
				unsigned int min_mahalanobis_index = 0;
				bool firstMahalanobis = true;
				saveGaussIndex(i,j); //Sets the last gauss index so that from now on we can pass just the gaussian, same for the image

				//Calculate the mahalanobis distance for each gaussian to the current pixel, find minimum mahalanobis distance, degenerate weights
				for (unsigned int k = 0; k < numGaussians; k++)
				{
					if (gauss(k).getWeight() < minWeight)
						gauss(k).clear();
					if (gauss(k).getFrames() > 0)
					{
						mahalanobis_distance[k] = gauss(k).calculateDistance(i,j,currentImage);
						if (firstMahalanobis)
						{
							firstMahalanobis = false;
							min_mahalanobis_index = k;
						}

						else if (mahalanobis_distance[k] < mahalanobis_distance[min_mahalanobis_index])
							min_mahalanobis_index = k;

						gauss(k).decWeight();// Degenerate the weight, keep in mind this is before we add alpha to the closest gaussian
					}
					else
						mahalanobis_distance[k] = -1;//We set the mahalanobis distance to -1 so that when outputted, we can tell that it wasn't calculated
				}

				if (mahalanobis_distance[min_mahalanobis_index] < minDistanceHighPass && !firstMahalanobis)
				{
					if (mahalanobis_distance[min_mahalanobis_index] < minDistanceThreshold)
						gauss(min_mahalanobis_index).calculateVariance(i,j,currentImage);
					gauss(min_mahalanobis_index).calculateMean(i,j,currentImage);
					gauss(min_mahalanobis_index).incWeight();
					gauss(min_mahalanobis_index).incFrames();

					if (gauss(min_mahalanobis_index).getWeight() > maxWeight)
						gauss(min_mahalanobis_index).setWeight(maxWeight);

					if(gauss(min_mahalanobis_index).getWeight() < weightThreshold)
					{
						differenceImage(i,j) = 255;
					}
				}
				else
				{
					differenceImage(i,j) = 255;
					unsigned int min_weight_index=0;
					for(unsigned int k = 1;k < numGaussians;k++)
					{
						if (gauss(k).getFrames() <= 0)
						{
							min_weight_index=k;
							break;
						}

						if(gauss(k).getWeight() < gauss(min_weight_index).getWeight())
							min_weight_index=k;
					}

					gauss(min_weight_index).setMu(i,j,currentImage);
					gauss(min_weight_index).setWeight(initialWeight);
					gauss(min_weight_index).setVariance(initialVariance);
					gauss(min_weight_index).setFrames(1);
				}
			}
		}
		imageRan = currentImage;
	}
};
#endif
