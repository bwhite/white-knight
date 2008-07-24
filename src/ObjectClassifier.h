/**
 * This class is an abstract base class that allows for generic binary classification
 *
 * \author Brandyn White
 */

#ifndef OBJECT_CLASSIFIER_H
#define OBJECT_CLASSIFIER_H

class ObjectClassifier
{
public:
	virtual void loadModel(const char* filename) = 0;//Given a file name, it will load that model
	virtual void saveModel(const char* filename) = 0;//Given a file name, it will save the model currently represented internally
	virtual void trainModel(double **sampleFeatureVectors, bool *samplePolarities, unsigned int numSamples, unsigned int sampleLength) = 0;//Given an array of pointers to data arrays, the number of arrays, their length, and a corresponding array of bools representing polarity
	virtual double classifyVector(double *featureVector, unsigned int sampleLength) = 0;
	virtual ~ObjectClassifier()
	{}
};
#endif


/*
Delete this when done


We need to create an abstract base for a classification system, most simple classifiers take in a small chip, calculate the
features for it, and output a probability vector corresponding to the probability of the chip being each object in the vector.  However, there are instances where other information is required
-Difference image for chip masking
-Many chips for optical flow calculation
-Entire tracks of chips

In all of these but having entire tracks of chips we have one 'test' image, and another way to input supporting information.  The problem with this is it blurs the distinction between the abstract class and the implementations.