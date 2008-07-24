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
#include <deque>
#include "../configTools/configReader.h"
#include "../ImageProvider.h"
#include "../BackgroundSubtraction.h"
#include "../GPU/GPUModule.h"
using namespace std;

class GrimGaussian
{
protected:
	float weight;
	float weightStdDevRatio;
	float *mu;    //Mean
	float variance; //sigma^2
	float rho; //Learning rate for weighted average
	//Config variables
	static unsigned int channels;
	static float alpha; //Learning rate for weights

public:
	static float getAlpha()
	{
		return alpha;
	}
	GrimGaussian()
	{
		mu = new float[channels];
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
			weightStdDevRatio = 10000;//TODO Change to be the max float value
		else
			weightStdDevRatio = (weight*weight)/variance;
	}
	
	//TODO Make this a lookup table, look into its correctness as well
	void computeRho(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		float nu = exp(-.5 * getSumDifferenceSquared(yPos, xPos, currentImage) / variance) / (pow(2*M_PI*variance,(float)channels/2.0));
		rho = nu*alpha;
	}
	
	inline float getRatio()
	{
		return weightStdDevRatio;
	}
	
	inline void setRho(unsigned int val)
	{
		rho = val;
	}

	inline void setVariance(unsigned int val)
	{
		variance = val;
	}

	static int loadConfig(std::list<vector<string> > configList)
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "Alpha", alpha);
		return notLoaded;
	}

	inline void setWeight(float weight)
	{
		this->weight = weight;
	}

	inline float getWeight()
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

	inline void setMu(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] = currentImage.read(yPos, xPos, channelIter);
	}
	
	inline void getMu(float * val)
	{
		memcpy(val,mu,3*sizeof(float));
	}
	
	inline void setMu(float *val)
	{
		memcpy(mu,val,3*sizeof(float));
	}

	inline void muToImage(unsigned int yPos,unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			currentImage.write(yPos, xPos, channelIter, (unsigned char)round(mu[channelIter]));
	}

	inline void setVariance(float variance)
	{
		this->variance = variance;
	}
	
	inline void getVariance(float *val)
	{
		(*val) = variance;
	}

	inline void clear()
	{
		weight = 0;
	}

	//Used many times in the algorithm, takes the difference of the current pixel vector - the mean vector, and transpose multiplies that by itself
	inline float getSumDifferenceSquared(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		float sum = 0;
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
		{
			float difference = currentImage.read(yPos, xPos, channelIter) - mu[channelIter];
			sum += (difference*difference);
		}
		return sum;
	}

	float calculateDistance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{	
		return getSumDifferenceSquared(yPos, xPos, currentImage) / (variance);//Basically mahalanobis distance squared and normalized for number of channels (to allow for the same 2.5 value to be used for any number of channels), this means we don't take the root of it which is compared to a thresh anyways (aka no accuracy change)
	}

	void calculateVariance(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		variance = (1 - rho) * variance + rho * getSumDifferenceSquared(yPos, xPos, currentImage);
	}

	void calculateMean(unsigned int yPos, unsigned int xPos, Image8 &currentImage)
	{
		for (unsigned int channelIter = 0; channelIter < channels; channelIter++)
			mu[channelIter] =  (1.0 - rho) * mu[channelIter] + rho * currentImage.read(yPos,xPos,channelIter); //This is a weighted average
	}
};

//Initialize static member variables
float GrimGaussian::alpha; //Learning rate for weights
unsigned int GrimGaussian::channels;//NOTE This is a hack around c++'s limitation on class inititalization with array new

class KGaussianGrimson : public BackgroundSubtraction
{
private:

	const unsigned int height;
	const unsigned int width;
	const unsigned int channels;
	Image8 currentImage;
	ImageProvider *imProvide;

	//GPU
	float *texture1;
	float *texture2;
	float *output;
	deque<unsigned char> distribution_matched_index_buffer;
	deque<unsigned int> x_coord_buffer;
	deque<unsigned int> y_coord_buffer;

	/***********Background Settings***********/
	unsigned int numGaussians;
	float initialWeight;
	float initialVariance;
	float minTotalBgWeight;
	unsigned int savedGaussIndex; //This is an optimization that also makes the code cleaner, we use this to save the last i/j index used

	/******************Thresholds********************/
	float minDistanceThreshold;

	GrimGaussian **gauss_data;//Where the gaussian map is held
	float *mahalanobis_distance;

	inline GrimGaussian& gauss(unsigned int height, unsigned int width, unsigned int k)
	{
		assert(height < this->height);
		assert(width < this->width);
		assert(k < this->numGaussians);//TODO Make a seperate assert type for these
		return *gauss_data[numGaussians * (height * this->width + width) + k];
	}
	
	void normalizeWeights(unsigned int height, unsigned int width)
	{
		float sum = 0;
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
		float tempSum = 0;
		
		float weightTotalSum = 0;
		
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
	inline GrimGaussian& gauss(unsigned int k)
	{
		assert(k < this->numGaussians);
		return *gauss_data[savedGaussIndex + k];
	}

public:
	KGaussianGrimson(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, std::list<vector<string> > configList): height(height), width(width), channels(channels), currentImage(height,width,channels)
	{
		this->imProvide = imProvide;
		loadConfig(configList);//Load all of the settings from the config file, thus replacing anything above that is listed there
		GrimGaussian::setChannels(channels);
		gauss_data = new GrimGaussian*[numGaussians * height * width];
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			gauss_data[memIter] = new GrimGaussian;
		mahalanobis_distance = new float[numGaussians];

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
				gauss(0).setWeight(initialWeight);
				for (unsigned int k = 1; k < numGaussians; k++)
				{
					gauss(k).setVariance(initialVariance);
					gauss(k).setWeight(0);
				}
				normalizeWeights(i,j);
			}
		}
		
		texture1 = new float[width*height*4];
		texture2 = new float[width*height*4];
		output = new float[width*height*4];
		GPUModule::Init(height, width);
	}

	~KGaussianGrimson()
	{
		delete [] texture1;
		delete [] texture2;
		delete [] output;
		
		GPUModule::Destroy();
		for (unsigned int memIter = 0; memIter < numGaussians*height*width; memIter++)
			delete gauss_data[memIter];
		delete [] mahalanobis_distance;
		delete [] gauss_data;
	}

	void loadConfig(std::list<vector<string> > configList)
	{
		float tempMinDist;
		int notLoaded = 0;
		
		notLoaded += FindValue(configList, "NumberOfGaussians", numGaussians);
		notLoaded += FindValue(configList, "InitialWeight", initialWeight);
		notLoaded += FindValue(configList, "InitialVariance", initialVariance);
		notLoaded += FindValue(configList, "MinDistanceThresh", tempMinDist);
		notLoaded += FindValue(configList, "MinTotalBgWeight", minTotalBgWeight);
		notLoaded += GrimGaussian::loadConfig(configList);
		if (notLoaded != 0)
			exit(0);
		minDistanceThreshold = tempMinDist * tempMinDist;
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
		assert(channels == 3);
		differenceImage.zero();
		imProvide->getImage(currentImage);
		
		float *tex1pos = texture1;//(CurMuR, CurMuG, CurMuB,Variance)
		float *tex2pos = texture2;//(CurR, CurG, CurB, *DUMMY*)
		unsigned int pixelsPacked = 0;
		
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
						
						
						/* This is the key part of the algorithm, here we are saying that if the pixel is within a certain number
						 * of std. dev. away from distribution k, then we say we have found a match, now if this k is less or equal to
						 * our weightIndThresh (B), then this pixel is part of our background
						 */	
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

				if (fgPixel)
					differenceImage.write(i,j,0,255);

				if (matchedDist && mahalanobis_distance[distribution_matched_index] < minDistanceThreshold)
				{
					/*gauss(distribution_matched_index).computeRho(i,j, currentImage);
					gauss(distribution_matched_index).calculateMean(i,j,currentImage);
					gauss(distribution_matched_index).calculateVariance(i,j,currentImage);*/
					gauss(distribution_matched_index).getMu(tex1pos);
					tex1pos += 3;
					gauss(distribution_matched_index).getVariance(tex1pos);
					tex1pos++;
					
					for (unsigned int tempChanIter = 0; tempChanIter < 3; tempChanIter++)
					{
						*tex2pos = currentImage.read(i,j,tempChanIter);
						tex2pos++;
					}
					tex2pos++;//NOTE Here we waste one float due to RGBA being faster than RGB
					
					distribution_matched_index_buffer.push_back(distribution_matched_index);
					x_coord_buffer.push_back(j);
					y_coord_buffer.push_back(i);
					pixelsPacked++;
					gauss(distribution_matched_index).incWeight();
				}
				else
				{
					unsigned int min_weight_index = numGaussians - 1;
					gauss(min_weight_index).setMu(i,j,currentImage);
					gauss(min_weight_index).setWeight(initialWeight);
					gauss(min_weight_index).setVariance(initialVariance);
				}
				//Since the normalize weights function is generally slow, we want to avoid unnecessary calls to it
				//So, if the matched distribution is the top distribution, and if its the only one that is BG, then we skip this
				if (distribution_matched_index != 0 || weightIndThresh != 0)
					normalizeWeights(i,j);		
					
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
		//End of double for loops
		float scanLines = ceil((double)pixelsPacked/(double)width);
		//cout << "OldMu: " << texture1[0] << " " <<texture1[1] << " " << texture1[2] << endl;
		//cout << "OldVar: " << texture1[3] << endl;
		GPUModule::UpdateGaussian(scanLines,GrimGaussian::getAlpha(), texture1, texture2, output);
		//cout << "NuMu: " << output[0] << " " <<output[1] << " " << output[2] << endl;
		//cout << "NuVar: " << output[3] << endl;
		assert(distribution_matched_index_buffer.size() == pixelsPacked);
		assert(x_coord_buffer.size() == pixelsPacked);
		assert(y_coord_buffer.size() == pixelsPacked);
		
		unsigned int i, j, distribution_matched_index;
		float *outputIter = output;
		for (unsigned int updatePixIter = 0; updatePixIter < pixelsPacked;updatePixIter++)
		{
			distribution_matched_index = distribution_matched_index_buffer.front();
			distribution_matched_index_buffer.pop_front();
			
			i = y_coord_buffer.front();
			y_coord_buffer.pop_front();
			
			j = x_coord_buffer.front();
			x_coord_buffer.pop_front();
			
			gauss(i,j,distribution_matched_index).setMu(outputIter);
			outputIter += 3;
			gauss(i,j,distribution_matched_index).setVariance(*outputIter);
			outputIter++;
			
			//We can skip the sort if our matched dist. is the first NOTE That this isn't exactly true, if the variance grows faster than weight declines
			if (distribution_matched_index != 0)
				sortDistributions(i,j);
		}
		
		imageRan.copyFrom(currentImage);
	}

};
#endif
