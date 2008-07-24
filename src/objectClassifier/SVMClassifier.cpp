#include "SVMClassifier.h"
#include <cassert>
SVMClassifier::SVMClassifier() : model(0), featureNodes(0), lastSampleLength(0)
{

}

SVMClassifier::~SVMClassifier()
{
	if (model)
	{
		svm_destroy_model(model);
		model = 0;
	}
	
	if (featureNodes)
	{
		delete [] featureNodes;
		featureNodes = 0;
		lastSampleLength = 0;
	}
}

void SVMClassifier::loadModel(const char* filename)//Given a file name, it will load that model
{
	//If we already have a model, remove it
	if (model)
	{
		svm_destroy_model(model);
		model = 0;
	}
	model = svm_load_model(filename);
}

void SVMClassifier::saveModel(const char* filename)//Given a file name, it will save the model currently represented internally
{
	if (model)
		svm_save_model(filename, model);
	else
		cerr << "SVM: Trying to save model...model not loaded!" << endl;
}

void SVMClassifier::trainModel(double **sampleFeatureVectors, bool *samplePolarities, unsigned int numSamples, unsigned int sampleLength)//Given an array of pointers to data arrays, the number of arrays, their length, and a corresponding array of bools representing polarity.  Note that this uses dense vectors!
{
	//If we already have a model, remove it
	if (model)
	{
		svm_destroy_model(model);
		model = 0;
	}	
	
	// default values from svm-train
	//TODO Made these modifiable
	struct svm_parameter param;
	param.svm_type = C_SVC;
	param.kernel_type = RBF;
	param.degree = 3;
	param.gamma = 0;        // 1/k
	param.coef0 = 0;
	param.nu = 0.5;
	param.cache_size = 100;
	param.C = 400;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 1; 
	param.probability = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;
	
	//Create svm_problem
	struct svm_problem problem;
	problem.l = numSamples;
	problem.y = new double[numSamples];
	problem.x = new struct svm_node*[numSamples];
	
	//Allocate space for the svm_nodes (basically a sparse representation of the vector,though we will use it as if it were dense).
	//One extra node for each sample must be allocated for the terminating "-1" index
	struct svm_node *x_space = new struct svm_node[numSamples*(sampleLength+1)];
	
	for (unsigned int sampleIter = 0; sampleIter < numSamples; sampleIter++)
	{
		unsigned int xSpaceOffset = sampleIter*(sampleLength+1);
		problem.y[sampleIter] = samplePolarities[sampleIter] ? 1.0 : -1.0;
		problem.x[sampleIter] = &x_space[xSpaceOffset];
		for (unsigned int dimensionIter = 0; dimensionIter < sampleLength; dimensionIter++)
		{
			x_space[xSpaceOffset+dimensionIter].index = dimensionIter + 1;
			x_space[xSpaceOffset+dimensionIter].value = sampleFeatureVectors[sampleIter][dimensionIter];
		}
		x_space[xSpaceOffset+sampleLength].index = -1;//Each of the sparse vectors must end with an index of -1
	}
	
	if (param.gamma == 0)
		param.gamma = 1.0/(double)sampleLength;
	
	model = svm_train(&problem,&param);

	assert(model);
	
	
	delete [] problem.x;
	problem.x = 0;
	delete [] problem.y;
	problem.y = 0;
}

double SVMClassifier::classifyVector(double *featureVector, unsigned int sampleLength)
{
	featureNodes = new svm_node[sampleLength + 1];
	
	for (unsigned int featureIter = 0; featureIter < sampleLength; featureIter++)
	{
		featureNodes[featureIter].value = featureVector[featureIter];
		featureNodes[featureIter].index = featureIter + 1;
	}
	
	featureNodes[sampleLength].index = -1;

	double probability;
	svm_predict_values(model,featureNodes,&probability);
	delete [] featureNodes;
	featureNodes = 0;
	return probability;
}
