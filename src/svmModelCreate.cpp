#include <iostream>
#include <ctime>
#include <vector>
#include "objectClassifier/SVMClassifier.h"
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "imageProvider/ChipLoader.h"
#include "imageTools/imageTools.h"
#include "objectClassifier/featureCalculator.h"
#include <opencv/cv.h>
#include <cmath>

using namespace std;

/* This function takes in what feature mode you want to use, the current sample (training image) number, whether or not the test vector size has been set, the size of the test vector if it has been set, the array of training polarities, the array of arrays of feature vectors, and one/two image providers (two if you want to use flow based features)
*/
void imageListFeatureCalculation(unsigned int chipHeight, unsigned int chipWidth, unsigned int chipChannels, unsigned int featureMode, unsigned int &curSample, bool &testVectorSizeSet, unsigned int &testVectorSize, bool *trainPolarities, double **trainVec, bool listPolarity, ChipLoader *imProvide, ChipLoader *imProvideTwin, bool getOneFrame)
{
	assert(imProvide);
	//Make sure that the two lists have the same number of images in them
	if (imProvideTwin)
	{
		assert(imProvide->getNumListedImages() == imProvideTwin->getNumListedImages());
	}
	

	//Create chip images
	Image8 *currentChip = new Image8(chipHeight,chipWidth,chipChannels);
	Image8 *currentChipTwin = 0;
	if (imProvideTwin)
	{
		currentChipTwin = new Image8(chipHeight,chipWidth,chipChannels);
	}
	while(1)
	{
		vector<double> currentVector;
		try
		{
			//Read in chip images from file lists
			imProvide->getImage(currentChip);
			if (imProvideTwin)
			{
				imProvideTwin->getImage(currentChipTwin);
			}
		}
		catch (NoMoreImages)
		{
			break;
		}

		featureCalculation(currentVector,featureMode, currentChip, currentChipTwin);

		if (testVectorSizeSet == false)
		{
			testVectorSize = currentVector.size();
			testVectorSizeSet = true;
		}
		if (testVectorSize != currentVector.size())
		{
			cerr << "There is a mismatch in the feature vector! TestVecSize: " << testVectorSize << " != CurrentVecSize: " << currentVector.size() << endl;
		}
		if (trainPolarities)
			trainPolarities[curSample] = listPolarity;
		trainVec[curSample] = new double[testVectorSize];
		memcpy(trainVec[curSample],&(currentVector[0]),sizeof(double) * testVectorSize);
		if (getOneFrame)
			break;

		curSample++;
	}
}

int main(int argc, char **argv)
{
	srand(time(0));
	if (argc != 2 && argc != 3)
	{
		cerr << "Error, arguments must be config file and optionally feature mode" << endl;
		cerr << argv[0] << " [config] (feature_mode)" << endl;
		exit(1);
	}
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	list<ImageFormat*> formats1;
	list<ImageFormat*> formats2;
	list<ImageFormat*> formats3;
	list<ImageFormat*> formats4;
	list<ImageFormat*> formats5;
	list<ImageFormat*> formats6;
	list<ImageFormat*> formats7;
	list<ImageFormat*> formats8;
	list<ImageFormat*> formats9;
	list<ImageFormat*> formats10;
	formats1.push_back(new Devil);
	formats2.push_back(new Devil);
	
	char positiveListName[100];
	char negativeListName[100];
	char positiveListTwinName[100];
	char negativeListTwinName[100];

	char testListPositiveName[100];
	char testListNegativeName[100];
	char testListPositiveTwinName[100];
	char testListNegativeTwinName[100];
	
	char testListFrameName[100];
	char testListFrameTwinName[100];

	double svmPositiveThreshold;
	unsigned int height, width, channels;
	unsigned int featureMode = 0;	

	unsigned int notLoaded = 0;
	notLoaded += FindValue(configList, "ChipHeight", height);
	notLoaded += FindValue(configList, "ChipWidth", width);
	notLoaded += FindValue(configList, "ChipChannels", channels);
	notLoaded += FindValue(configList, "PositiveThreshold", svmPositiveThreshold);
	if (argc != 3)
		notLoaded += FindValue(configList, "FeatureMode", featureMode);
	else
		sscanf(argv[2]," %u ",&featureMode);
	notLoaded += FindValue(configList, "PositiveList", positiveListName);
	notLoaded += FindValue(configList, "NegativeList", negativeListName);
	cout << "---Using Feature Mode " << featureMode << endl;
	assert(!notLoaded);

	bool positiveTwinPresent = !FindValue(configList, "PositiveTwinList", positiveListTwinName);
	bool negativeTwinPresent = !FindValue(configList, "NegativeTwinList", negativeListTwinName);

	char modelInputPath[100];
	bool loadModel = !(FindValue(configList, "SVMInputModel", modelInputPath));

	bool testSetPositivePresent = !FindValue(configList, "PositiveTestList", testListPositiveName);
	bool testSetNegativePresent = !FindValue(configList, "NegativeTestList", testListNegativeName);

	bool testSetPositiveTwinPresent = !FindValue(configList, "PositiveTwinTestList", testListPositiveTwinName);
	bool testSetNegativeTwinPresent = !FindValue(configList, "NegativeTwinTestList", testListNegativeTwinName);

	bool testFramePresent = !FindValue(configList, "TestFrames", testListFrameName);
	bool testFrameTwinPresent = !FindValue(configList, "TestTwinFrames", testListFrameTwinName);

/*
	TODO Input pairs of whole frames, go through at a reasonable step size, and at a few scales and output all positive chips (by the threshold), do this after model training, output number of positive chips
*/
	bool useFlow = false;

	ChipLoader* imProvidePositive = new ChipLoader(configList, formats1,positiveListName);
	ChipLoader* imProvideNegative = new ChipLoader(configList, formats2,negativeListName);
	ChipLoader* imProvidePositiveTest = 0;
	ChipLoader* imProvideNegativeTest = 0;

	//For flow calculation
	ChipLoader* imProvidePositiveTwin = 0;
	ChipLoader* imProvideNegativeTwin = 0;
	ChipLoader* imProvidePositiveTestTwin = 0;
	ChipLoader* imProvideNegativeTestTwin = 0;

	//For Frame Testing
	ChipLoader* imProvideTestFrame = 0;
	ChipLoader* imProvideTestFrameTwin = 0;

	if (positiveTwinPresent)
	{
		useFlow = true;
		assert(negativeTwinPresent);
		formats5.push_back(new Devil);
		formats6.push_back(new Devil);
		imProvidePositiveTwin = new ChipLoader(configList, formats5,positiveListTwinName);
		imProvideNegativeTwin = new ChipLoader(configList, formats6,negativeListTwinName);
	}

	if (testSetPositivePresent)
	{
		if (useFlow)
		{
			assert(testSetPositiveTwinPresent);
			formats7.push_back(new Devil);
			imProvidePositiveTestTwin = new ChipLoader(configList, formats7,testListPositiveTwinName);
		}
		formats3.push_back(new Devil);
		imProvidePositiveTest = new ChipLoader(configList, formats3,testListPositiveName);
	}

	if (testSetNegativePresent)
	{
		if (useFlow)
		{
			assert(testSetNegativeTwinPresent);
			formats8.push_back(new Devil);
			imProvideNegativeTestTwin = new ChipLoader(configList, formats8,testListNegativeTwinName);
		}
		formats4.push_back(new Devil);
		imProvideNegativeTest = new ChipLoader(configList, formats4,testListNegativeName);
	}

	if (testFramePresent)
	{
		if (useFlow)
		{
			assert(testFrameTwinPresent);
			formats10.push_back(new Devil);
			imProvideTestFrameTwin = new ChipLoader(configList, formats10,testListFrameTwinName);
		}
		formats9.push_back(new Devil);
		imProvideTestFrame = new ChipLoader(configList, formats9,testListFrameName);
	}

	assert(channels == 1);
	
	Image8 currentChip(height,width,channels);
	cout << "#Height: " << height << " Width: " << width << endl;
	double probability;
	SVMClassifier svm;
	Devil devil;

	vector<vector<double> > featureVectors;
	unsigned int numSamples = imProvidePositive->getNumListedImages() + imProvideNegative->getNumListedImages();
	unsigned int testVectorSize = 0;
	bool testVectorSizeSet = false;
	double **trainVec = new double*[numSamples];

	memset(trainVec,0,numSamples*sizeof(double*));

	bool *trainPolarities = new bool[numSamples];
	unsigned int curSample = 0;

	if (loadModel)
	{
		cout << "Loading model from file " << modelInputPath << endl;
		svm.loadModel(modelInputPath);
	}
	else
	{
		imageListFeatureCalculation(height,width,channels,featureMode, curSample, testVectorSizeSet, testVectorSize, trainPolarities, trainVec, true, imProvidePositive,imProvidePositiveTwin);
		imageListFeatureCalculation(height,width,channels,featureMode, curSample, testVectorSizeSet, testVectorSize, trainPolarities, trainVec, false, imProvideNegative,imProvideNegativeTwin);
	
		#ifndef NDEBUG
		cout << "NumSamples " << numSamples << " VecSize " << testVectorSize << endl;
		#endif

		//Save current input vectors to a file so that it can be trained with separate SVM parameters, etc
		if (testVectorSizeSet != false && numSamples != 0)
		{
			FILE * svmPreModel = fopen("svm_pre_model.txt","w");
			for (unsigned int vecIter = 0; vecIter < numSamples; vecIter++)//For each vector
			{
				if (trainPolarities[vecIter])
					fprintf(svmPreModel,"+1");
				else
					fprintf(svmPreModel,"-1");
				
				for (unsigned int dimIter = 1; dimIter <= testVectorSize; dimIter++)//For each dimension
				{
					fprintf(svmPreModel," %u:%.6f",dimIter,trainVec[vecIter][dimIter]);
				}
			
				fprintf(svmPreModel,"\n");
			}
			fclose(svmPreModel);
		}
		//Train the model and save it
		svm.trainModel(trainVec, trainPolarities, numSamples, testVectorSize);
		svm.saveModel("svm.model");
	}


	FILE * decisionProbFile = fopen("output/decision_prob.txt","w");
	assert(decisionProbFile);
	unsigned int correctClassPos = 0;
	unsigned int incorrectClassPos = 0;
	unsigned int correctClassNeg = 0;
	unsigned int incorrectClassNeg = 0;
	if (testSetPositivePresent)
	{
		double *testVec = 0;
		while(1)
		{
			unsigned int dummySample = 0;
			imageListFeatureCalculation(height,width,channels,featureMode, dummySample, testVectorSizeSet, testVectorSize, 0, &testVec, true, imProvidePositiveTest,imProvidePositiveTestTwin,true);
			if (testVec)
			{
				probability = svm.classifyVector(testVec,testVectorSize);
				fprintf(decisionProbFile,"+1 %f\n",probability);
				cout << "PosTest: " << probability << endl;
				if (probability >= svmPositiveThreshold)
					correctClassPos++;
				else
					incorrectClassPos++;
				delete [] testVec;
				testVec = 0;
			}
			else
				break;
		}
		cout << "TP: " << correctClassPos << " FN: " << incorrectClassPos << endl;
		delete imProvidePositiveTest;
		if (imProvidePositiveTestTwin)
			delete imProvidePositiveTestTwin;
	}

	if (testSetNegativePresent)
	{

		double *testVec = 0;
		while(1)
		{
			unsigned int dummySample = 0;
			imageListFeatureCalculation(height,width,channels,featureMode, dummySample, testVectorSizeSet, testVectorSize, 0, &testVec, true, imProvideNegativeTest,imProvideNegativeTestTwin,true);
			if (testVec)
			{
				probability = svm.classifyVector(testVec,testVectorSize);
				fprintf(decisionProbFile,"-1 %f\n",probability);
				cout << "NegTest: " << probability << endl;
				if (probability < svmPositiveThreshold)
					correctClassNeg++;
				else
					incorrectClassNeg++;
				delete [] testVec;
				testVec = 0;
			}
			else
				break;
		}
		cout << "TN: " << correctClassNeg << " FP: " << incorrectClassNeg << endl;
		delete imProvideNegativeTest;
		if (imProvideNegativeTestTwin)
			delete imProvideNegativeTestTwin;
	}

	fclose(decisionProbFile);

	if (testSetNegativePresent && testSetPositivePresent)
	{
		cout << "-----Total Results-----" << endl;
		cout << "TP: " << correctClassPos << " FN: " << incorrectClassPos << endl;
		cout << "TN: " << correctClassNeg << " FP: " << incorrectClassNeg << endl;
		cout << "Precision: " << correctClassPos / (double)(correctClassPos + incorrectClassNeg) << endl;
		cout << "Recall: " << correctClassPos / (double)(correctClassPos + incorrectClassPos) << endl;
	}
	
	delete [] trainPolarities;
	for (unsigned int vecIter = 0; vecIter < numSamples; vecIter++)
	{
		if (trainVec[vecIter])
			delete [] trainVec[vecIter];
	}

	delete [] trainVec;


	//TODO For each frame in frame list, use a list of scales/strides and take each chip, test against the model, if positive save to the positive folder with an incrementing name (with some decorators such as settings/time/etc)
	//This can be used several ways, 1) one to find false positives by passing in frames that just have negatives and putting the false positives back into training
	//2) To collect more positive chips by giving it a large number of frames with lots of people (possibly moving camera), and set the positive threshold low (then manually collect real people)

	//Create temp chips
	if (testFramePresent)//NOTE Since we are using chiploader, the frames will be resized on load
	{
		//TODO Get frame size, allocate temp frames
		Devil devil;
		unsigned int frameHeight, frameWidth, frameChannels;
		imProvideTestFrame->getImageInfo(frameHeight,frameWidth, frameChannels);
		Image8 *currentFrame = new Image8(frameHeight,frameWidth,frameChannels);
		Image8 *currentFrameTwin = 0;

		Image8 *currentChip = new Image8(height,width,channels);
		Image8 *currentChipTwin = 0;
		double *testVec = new double[testVectorSize];
		unsigned int frameCounter = 0;
		char tempName[100];

		cout << "Frame - Height: " << frameHeight << " Width: " << frameWidth << " Channels: " << frameChannels << " Chip - Height: " << height << " Width: " << width << " Channels: " << channels << endl;		

		if (testFrameTwinPresent)
		{
			currentFrameTwin = new Image8(frameHeight,frameWidth,frameChannels);
			currentChipTwin = new Image8(height,width,channels);
		}
		while(1)
		{
			try
			{
				//Read in chip images from file lists
				imProvideTestFrame->getImage(currentFrame);
				if (imProvideTestFrameTwin)
				{
					imProvideTestFrameTwin->getImage(currentFrameTwin);
				}
			}
			catch (NoMoreImages)
			{
				break;
			}
			//For every chip in the frame
			unsigned int chipStride = 10;
			for (unsigned int yIter = 0; yIter < frameHeight - height; yIter+=chipStride)
				for (unsigned int xIter = 0; xIter < frameWidth - width; xIter+=chipStride)
				{
					unsigned int chipCounter = 0;
					vector<double> currentVector;
					//Use subimage to get chip from frame at the current yIter/xIter position
					SubImage(*currentFrame, *currentChip, xIter, yIter);
					if (testFrameTwinPresent)
						SubImage(*currentFrameTwin, *currentChipTwin, xIter, yIter);

					featureCalculation(currentVector,featureMode, currentChip, currentChipTwin);
		
					if (testVectorSizeSet == false)
					{
						testVectorSize = currentVector.size();
						testVectorSizeSet = true;
					}

					if (testVectorSize != currentVector.size())
					{
						cerr << "There is a mismatch in the feature vector! TestVecSize: " << testVectorSize << " != CurrentVecSize: " << currentVector.size() << endl;
					}

					memcpy(testVec,&(currentVector[0]),sizeof(double) * testVectorSize);

					probability = svm.classifyVector(testVec,testVectorSize);
					//TODO If positive, save chip to file with decorated name
					if (probability >= 1.0/*svmPositiveThreshold*/)
					{
						sprintf(tempName, "output/test_chips/%.4f-%.6u-%.6u-%.6u-%.6u-%.6u.jpg",probability,featureMode,frameCounter,chipCounter, xIter, yIter);
						devil.writeImage(*currentChip, tempName);
						sprintf(tempName, "output/test_chips/%.4f-%.6u-%.6u-%.6u-%.6u-%.6uT.jpg",probability,featureMode,frameCounter,chipCounter, xIter, yIter);
						devil.writeImage(*currentChipTwin, tempName);
						cout << "Positive @ (" << xIter << "," << yIter << ")" << " - " << probability << endl;
					}
					chipCounter++;
				}
			frameCounter++;
		}
		if (testVec)
		{
			delete [] testVec;
			testVec = 0;
		}
		delete currentFrame;
		delete currentChip;
		delete currentChipTwin;
		delete currentFrameTwin;
	}


	delete imProvidePositive;
	delete imProvideNegative;
	if (imProvidePositiveTwin)
		delete imProvidePositiveTwin;
	if (imProvideNegativeTwin)
			delete imProvideNegativeTwin;
	if (imProvideTestFrame)
		delete imProvideTestFrame;
	if (imProvideTestFrameTwin)
		delete imProvideTestFrameTwin;
	return 0;
}
