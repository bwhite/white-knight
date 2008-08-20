/**
 * This class is an implementation of the ObjectClassifier that allows for binary classification
 *
 * \author Brandyn White
 */

#ifndef SVM_CLASSIFIER_H
#define SVM_CLASSIFIER_H
#include <iostream>
#include "libsvm-2.83/svm.h"

using namespace std;

class SVMClassifier
{
public:
	SVMClassifier();
	virtual void loadModel(const char* filename);//Given a file name, it will load that model and set the size
	virtual void saveModel(const char* filename);//Given a file name, it will save the model currently represented internally
	virtual void trainModel(double **sampleFeatureVectors, bool *samplePolarities, unsigned int numSamples, unsigned int sampleLength);//Given an array of pointers to data arrays, the number of arrays, their length, and a corresponding array of bools representing polarity
	virtual double classifyVector(double *featureVector, unsigned int sampleLength);
	virtual ~SVMClassifier();
private:
	struct svm_model *model;
	struct svm_node *featureNodes;
	unsigned int lastSampleLength;
};
#endif
