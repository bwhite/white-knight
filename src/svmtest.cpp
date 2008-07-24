#include <iostream>
#include <ctime>
#include "objectClassifier/SVMClassifier.h"

using namespace std;

int main()
{	
	srand(time(0));
	SVMClassifier svm;
	
	//Do a simple test of loading a pre-made model, testing 2 vectors, and saving it
	double testVectorPos[] = {.7,1,1,-.32,-.105,-1,1,-.4198,-1,-.2258,1,-1};
	double testVectorNeg[] = {.58,-1,.3333,-.6,1,-1,1,.358779,-1,-.484,-1,1};
	unsigned int testVectorSize = 12;
	svm.loadModel("svm.model");
	
	double probability;
	probability = svm.classifyVector(testVectorPos,testVectorSize);
	cout << "Test VectorPos Probability is " << probability << endl;
	
	probability = svm.classifyVector(testVectorNeg,testVectorSize);
	cout << "Test VectorNeg Probability is " << probability << endl;
	svm.saveModel("svmNew1.model");
	
	//Create a new model that exists of a simple cluster in the second quadrant of a 2d grid being positive, and the rest being negative (x,y)
	testVectorSize = 2;
	unsigned int numSamples = 1000;
	double **trainVec = new double*[numSamples];
	bool *trainPolarities = new bool[numSamples];
	for (unsigned int vecIter = 0; vecIter < numSamples; vecIter++)
	{
		trainVec[vecIter] = new double[2];
		//Pos
		if (vecIter <= (numSamples - 1)/2)
		{
			trainVec[vecIter][0] = -1*rand()/(double)RAND_MAX;
			trainVec[vecIter][1] = rand()/(double)RAND_MAX;
			trainPolarities[vecIter] = true;
		}
		//Neg
		else
		{
			trainVec[vecIter][0] = rand()/(double)RAND_MAX;
			trainVec[vecIter][1] = -1*rand()/(double)RAND_MAX;
			trainPolarities[vecIter] = false;
		}
	}
	
	svm.trainModel(trainVec, trainPolarities, numSamples, testVectorSize);
	svm.saveModel("svmNew2.model");
	
	for (unsigned int testVecIter = 0; testVecIter < numSamples; testVecIter++)
	{
		probability = svm.classifyVector(trainVec[testVecIter],testVectorSize);
		cout << "Test Probability is " << probability << endl;
	}
	
	delete [] trainPolarities;
	for (unsigned int vecIter = 0; vecIter < numSamples; vecIter++)
		delete [] trainVec[vecIter];
	delete [] trainVec;
	
	return 0;
}