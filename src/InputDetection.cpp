#include "backgroundSubtraction/kgaussgrimson.h"
//#include "backgroundSubtraction/kgausspower.h"
#include "imageTools/connectedComponents.h"
#include "imageTools/binaryImageTools.h"
#include "imageProvider/OpenCVReader.h"
//#include "backgroundSubtraction/kgaussiansubtraction.h"
#include "backgroundSubtraction/NonParametric.h"
#include "InputDetection.h"
#include "imageTools/imageTools.h"
#include "imageTools/font.h"
//#include "backgroundSubtraction/kgausspower.h"
#include "imageTools/ChangeDetector.h"
#include <cstdio>

void GradientMagnitudeDirection(const Image8 &inputImage, std::vector<double> &featureVector)
{
	assert(inputImage.getChannels() == 1);
    double sumx, sumy, mag;
    unsigned int width = inputImage.getWidth();
    unsigned int height = inputImage.getHeight();

    for (unsigned int i = 1; i < height - 1; i++)
            for (unsigned int j = 1; j < width - 1; j++)
            {
                    sumx = -inputImage.read(i-1,j-1) + inputImage.read(i-1,j+1) - 2 * inputImage.read(i,j-1) + 2 * inputImage.read(i,j+1) - inputImage.read(i+1,j-1) + inputImage.read(i+1,j+1);
                    sumy = inputImage.read(i-1,j-1) + 2 * inputImage.read(i-1,j) + inputImage.read(i-1,j+1) - inputImage.read(i+1,j-1) - 2 * inputImage.read(i+1,j) - inputImage.read(i+1,j+1);
                    mag = sqrt(sumx*sumx + sumy*sumy);
                    featureVector.push_back(sumx / 1020);
                    featureVector.push_back(sumy / 1020);
                    featureVector.push_back(mag / 1442.5);
            }
}

void heightWidthRatio(const Image8 &inputImage, std::vector<double> &featureVector)
{
        featureVector.push_back((double)inputImage.getHeight()/(double)inputImage.getWidth());
}

InputDetection::InputDetection(string configPath) : imProvide(0), bgSub(0), diffImage(0), tempMorphologicalImage(0), tempGrayImage(0), fgEdge(0), bgEdge(0), currentImage(0), tempColorImage(0), forceBGMask(0), devil(0), frameCounter(0), boxImage(0), printedHeader(false), svmChip(40,40,1)
{
	list<vector<string> > configList;
	char fileName[100];
	strcpy(fileName, configPath.c_str());
	ReadConfig(fileName, configList);
	setup(configList);
}

InputDetection::InputDetection(list<vector<string> > configList) : imProvide(0), bgSub(0), diffImage(0), tempMorphologicalImage(0), tempGrayImage(0), fgEdge(0), bgEdge(0), currentImage(0), tempColorImage(0), forceBGMask(0), devil(0), frameCounter(0), boxImage(0), printedHeader(false), svmChip(40,40,1)
{	
	setup(configList);
}

InputDetection::~InputDetection()
{
	fclose(featureFile);
	if (bgSub)
	{
		delete bgSub;
		bgSub = 0;
	}	
	
	if (imProvide)
	{
		delete imProvide;
		imProvide = 0;
	}
	
	if (diffImage)
	{
		delete diffImage;
		diffImage = 0;
	}
	
	if (currentImage)
	{
		delete currentImage;
		currentImage = 0;
	}

	if (boxImage)
	{
		delete boxImage;
		boxImage = 0;
	}

	if (devil)
	{
		delete devil;
		devil = 0;
	}
	
	if (fgEdge)
	{
		delete fgEdge;
		fgEdge = 0;
	}
	
	if (bgEdge)
	{
		delete bgEdge;
		bgEdge = 0;
	}
	
	if (tempMorphologicalImage)
	{
		delete tempMorphologicalImage;
		tempMorphologicalImage = 0;
	}
	
	if (tempColorImage)
	{
		delete tempColorImage;
		tempColorImage = 0;
	}
	
	if (tempGrayImage)
	{
		delete tempGrayImage;
		tempGrayImage = 0;
	}
}

void InputDetection::setup(list<vector<string> > configList)
{
	devil = new Devil;
	char inputMode[100];
	unsigned int notLoaded = 0;
	
	//We want to provide a list of valid modes so that we can output them if a valid option isn't selected
	notLoaded += FindValue(configList, "InputMode", inputMode);
	vector<char*> validInputModes(2);
	validInputModes[0] = "cv";
	validInputModes[1] = "list";
	
	if (!strcasecmp(inputMode,validInputModes[0]))
	{
		imProvide = new OpenCVReader(configList);
	}
	else if (!strcasecmp(inputMode,validInputModes[1]))
	{
		list<ImageFormat*> formats;
		formats.push_back(new Devil);
		imProvide = new ListLoader(configList, formats);
	}
	else
	{
		cerr << "You must select an input mode! (case insensitive) Valid options are...\n";
		for (vector<char*>::iterator modeIter = validInputModes.begin();modeIter != validInputModes.end(); modeIter++)
			cout << *modeIter << endl;
	}
	assert(imProvide);
	
	
	char ilumFilename[100];

	notLoaded += FindValue(configList, "MinObjectArea", minObjectArea);
	notLoaded += FindValue(configList, "BlurIters", blurIters);
	notLoaded += FindValue(configList, "BlurThreshold", blurThreshold);
	notLoaded += FindValue(configList, "GlobalIlluminationThreshold", globalIlluminationThreshold);
	notLoaded += FindValue(configList, "FileList", ilumFilename);
	notLoaded += FindValue(configList, "JoinComponentsX", joinComponentsX);
	notLoaded += FindValue(configList, "JoinComponentsY", joinComponentsY);
	int boolConvert;
	notLoaded += FindValue(configList, "ChangeDetectionCreate", boolConvert);
	bool createIlluminationVec = boolConvert;

	notLoaded += FindValue(configList, "ChangeDetectionEnable", boolConvert);
	bool enableIlluminationDetect = boolConvert;

	strcat(ilumFilename,".ilum");	
	FILE *ilumFile = fopen(ilumFilename,"r+");

	featureFile = fopen("output/feature.track","w");
	if (!featureFile)
	{
		cerr << "Couldn't create file output/feature.track!" << endl;
	}
	
	svmCar.loadModel("car.model");
	svmPerson.loadModel("person.model");

	unsigned int height, width, channels;
	imProvide->getImageInfo(height, width, channels);	

	if (enableIlluminationDetect)
	{

		if (createIlluminationVec)
		{
			//NOTE - ANDREW's Illumination change detector
			ChangeDetector changeDetect(configList, imProvide);
			changeDetect.detectChanges(illuminationProbabilities);	
			//We want to load everything up and save it so that we don't have to keep calculating it, we will use the name of the filelist with a .ilum for this
			for (unsigned int ilumIter = 0; ilumIter < illuminationProbabilities.size(); ilumIter++)
				fprintf(ilumFile,"%f ",illuminationProbabilities[ilumIter]);
		}
		else
		{
			while (!feof(ilumFile))
			{
				double tempDouble;
				fscanf(ilumFile,"%lf ",&tempDouble);
				illuminationProbabilities.push_back(tempDouble);
			}
		}
		fclose(ilumFile);
	}

	diffImage = new Image8(height,width,1);
	currentImage = new Image8(height,width,3);
	boxImage = new Image8(height,width,3);
	tempColorImage = new Image8(height,width,3);
	tempMorphologicalImage = new Image8(height,width,1);
	bgEdge = new Image8(height,width,1);
	fgEdge = new Image8(height,width,1);
	tempGrayImage = new Image8(height,width,1);

	//bgSub = new KGaussianPower(height,width,channels,imProvide,configList);
	//bgSub = new NonParametric(height,width,channels,imProvide,configList);
	bgSub = new KGaussianGrimson(height,width,channels,imProvide,configList);
	assert(bgSub);
	font = new Font();
}

//Name is null terminated, feature is not (specify its size)
inline vector<string> makeFeature(const char* name, const char* featureData, unsigned int featureSize)
{
	vector<string> feature(2);
	//Name
	feature[0] = name;

	//FeatureData
	feature[1].assign(featureData,featureSize);//NOTE the documentation isn't clear how this works, so if something odd is happening with feature truncation, then look here!
	return feature;
}

//Takes in a list of vectors that represent the coordinates of each box and its label (tl_x,tl_y, br_x,br_y,label)
void InputDetection::outputData(list<vector<unsigned int> > box_data)
{
	unsigned char red[3] = {0,255,0};
	unsigned char blue[3] = {0,0,255};
	char imageText[100];
	boxImage->copyFrom(*currentImage);
	//NOTE Keep in mind that all of these assume that we are calling output data for the current frame
	if (frameCounter < illuminationProbabilities.size() && illuminationProbabilities[frameCounter] > globalIlluminationThreshold)
	{
		sprintf(imageText,"change");
		font->DrawText(*boxImage, imageText, 0, 0, blue);
	}
	for (list<vector<unsigned int> >::iterator boxIter = box_data.begin(); boxIter != box_data.end(); boxIter++)
	{
		//Save chip
		Image8 tempImage((*boxIter)[3]-(*boxIter)[1],(*boxIter)[2]-(*boxIter)[0],3);
		SubImage(*currentImage,tempImage,(*boxIter)[0],(*boxIter)[1]);
		char tempName[100];
		sprintf(tempName, "output/chips/%.6u-%.6u.jpg",(*boxIter)[4],frameCounter);
		devil->writeImage(tempImage, tempName);
		
		//Draw box
		DrawBox(*boxImage,(*boxIter)[0], (*boxIter)[1], (*boxIter)[2], (*boxIter)[3],red);

		//Draw text
		sprintf(imageText,"%u",(*boxIter)[4]);
		font->DrawText(*boxImage, imageText, (*boxIter)[0], (*boxIter)[1], red);
	}
	char tempName[100];
	sprintf(tempName, "output/box/%.6u.jpg",frameCounter);
	devil->writeImage(*boxImage, tempName);
}

vector<vector<vector<string> > > InputDetection::getObjects() throw (NoMoreImages)
{
	//Here we can hint the subtraction with different things
	if (frameCounter < illuminationProbabilities.size() && illuminationProbabilities[frameCounter] > globalIlluminationThreshold)
		bgSub->hint("GlobalIlluminationChange", 1.0);
	//TODO Force background hint
	//TODO Foreground hint
	
	bgSub->subtract(*diffImage,*currentImage);
	bgSub->resetHints();
	vector<vector<vector<string> > > objects;
	assert(currentImage->getChannels() == 3);
	//Cleanup subtraction image stage
	//TODO Make this configurable so that you can use erode, dilate, hole filling, blur, threshold, join components, (and), and (or) in different stages (create a config format, then at setup populate a list of function pointers and arguments for each image to be passed through)
	//Blur,threshold
	char tempName[100];
	//TODO All of these images need to be output into their own directories and be able to be turned off selectively
	sprintf(tempName, "output/diff/%.6u.png",frameCounter);
	//devil->writeImage(*diffImage, tempName);
	
	//NOTE that this blur introduces the limitation that the output images are only correct outside of 2 pixels from border
	//TODO Think about ways of removing this such as 0/1/neighbor extension
	for (unsigned int blurIter = 0; blurIter < blurIters; blurIter++)
		skipsmGaussBlur5(*diffImage, *diffImage);
	
	//Clear a 2 pixels on every edge (as the blur doesn't place any new values there)
	for (unsigned int yRemovePixIter = 0; yRemovePixIter < diffImage->getHeight(); yRemovePixIter++)
	{
		for (unsigned int xRemovePixIter = 0; xRemovePixIter < 2; xRemovePixIter++)
			diffImage->write(yRemovePixIter,xRemovePixIter,0,0);
		for (unsigned int xRemovePixIter = diffImage->getWidth()-2; xRemovePixIter < diffImage->getWidth(); xRemovePixIter++)
			diffImage->write(yRemovePixIter,xRemovePixIter,0,0);
	}
	for (unsigned int xRemovePixIter = 0; xRemovePixIter < diffImage->getWidth(); xRemovePixIter++)
	{
		for (unsigned int yRemovePixIter = 0; yRemovePixIter < 2; yRemovePixIter++)
			diffImage->write(yRemovePixIter,xRemovePixIter,0,0);
		for (unsigned int yRemovePixIter = diffImage->getHeight()-2; yRemovePixIter < diffImage->getHeight(); yRemovePixIter++)
			diffImage->write(yRemovePixIter,xRemovePixIter,0,0);
	}
	
	for (unsigned int yRemovePixIter = 0; yRemovePixIter < diffImage->getHeight(); yRemovePixIter++)
		for (unsigned int xRemovePixIter = 0; xRemovePixIter < diffImage->getWidth(); xRemovePixIter++)
		{
			if (yRemovePixIter >= diffImage->getHeight()-2 || xRemovePixIter >= diffImage->getWidth()-2 || yRemovePixIter < 2 || xRemovePixIter < 2)
				diffImage->write(yRemovePixIter,xRemovePixIter,0,0);
		}
	sprintf(tempName, "output/current/%.6u.jpg",frameCounter);
	devil->writeImage(*currentImage, tempName);
		
	sprintf(tempName, "output/blur/%.6u.png",frameCounter);
	//devil->writeImage(*diffImage, tempName);
	
	
	thresholdImage(*diffImage, blurThreshold);
	tempMorphologicalImage->copyFrom(*diffImage);
	joinComponents(joinComponentsX, joinComponentsY, *tempMorphologicalImage, *diffImage, 0, diffImage->getHeight(), 0, diffImage->getWidth());
	sprintf(tempName, "output/join/%.6u.png",frameCounter);
	//devil->writeImage(*diffImage, tempName);	


	//calculate the foreground edges to allow for foreground edge ratio calculation
	//bgSub->getBackgroundImage(*tempColorImage);
	RGB2Gray(*tempColorImage,*tempMorphologicalImage);
	Sobel(*tempMorphologicalImage,*bgEdge,0,getHeight(),0,getWidth());//Calculate BG Edge
	RGB2Gray(*currentImage,*tempMorphologicalImage);
	Sobel(*tempMorphologicalImage,*tempGrayImage,0,getHeight(),0,getWidth());//Calculate Current Edge
	Subtract(*tempGrayImage,*bgEdge, *fgEdge,0,getHeight(),0,getWidth());

	sprintf(tempName, "output/fgedge/%.6u.png",frameCounter);
	//devil->writeImage(*fgEdge, tempName);
	


	//Create list of objects, each represented by a list of features, each represented by a array[2] of vector<char>'s (name = [0] and data = [1])

	list<list<connectedPoint_t> > connectedObjects;
	getConnectedObjects(*diffImage, connectedObjects);
	//Populate objects
	for (list< list<connectedPoint_t> >::iterator objIter = connectedObjects.begin(); objIter != connectedObjects.end(); objIter++)
	{
		vector<vector<string> > object;
		//START Calculate features
		unsigned int area  = objIter->size();
		
		if (area < minObjectArea)
			continue;
		
		unsigned int tl_x = diffImage->getWidth();//These 4 are initialized to their max opposite values to force the object to initialize them
		unsigned int tl_y = diffImage->getHeight();
		unsigned int br_x = 0;
		unsigned int br_y = 0;
		double cent_x;
		double cent_y;
		double meanGrayValue;
		
		double sum_gray_color = 0;
		unsigned int sum_x = 0;
		unsigned int sum_y = 0;
		unsigned int sumFGEdge = 0;
		for (list<connectedPoint_t>::iterator pointIter = objIter->begin(); pointIter != objIter->end(); pointIter++)
		{
			sum_x += pointIter->x;
			sum_y += pointIter->y;
			sum_gray_color += .299 * currentImage->read(pointIter->y, pointIter->x, 0) + .587 * currentImage->read(pointIter->y, pointIter->x, 1) + .114 * currentImage->read(pointIter->y, pointIter->x, 2);
			
			if (tl_x > pointIter->x)
				tl_x = pointIter->x;
				
			if (tl_y > pointIter->y)
				tl_y = pointIter->y;
				
			if (br_x < pointIter->x)
				br_x = pointIter->x;
				
			if (br_y < pointIter->y)
				br_y = pointIter->y;
			sumFGEdge += (fgEdge->read(pointIter->y,pointIter->x) > fgEdgeThresh);
		}
		
		cent_x = (double)sum_x / (double)area;
		cent_y = (double)sum_y / (double)area;
		meanGrayValue = sum_gray_color / (double)area;
		
		unsigned int height = br_y - tl_y;
		unsigned int width = br_x - tl_x;
		

		//Calulate SVM Person and Car Probabilities
		Image8 tempImage(height,width,3);
		Image8 tempImageGray(height,width,1);
		SubImage(*currentImage,tempImage,tl_x,tl_y);
		//gray tempImage
		RGB2Gray(tempImage,tempImageGray);
		//resize tempImage to svmChip
		BilinearResample(tempImageGray,svmChip);

		vector<double> currentVector;
		GradientMagnitudeDirection(svmChip, currentVector);
		heightWidthRatio(svmChip, currentVector);
		double personProbability = svmPerson.classifyVector(&(currentVector[0]),currentVector.size());
		double carProbability = svmCar.classifyVector(&(currentVector[0]),currentVector.size());

		//END Calculate features
	
		//BEGIN Populate features - The convention for this is [0] = name, [1] = data
		char tempData[1000];
		int charWritten;
	
		charWritten = sprintf(tempData,"%u",area);
		assert(charWritten >= 0);
		object.push_back(makeFeature("area",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%u",height);
		assert(charWritten >= 0);
		object.push_back(makeFeature("height",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%u",width);
		assert(charWritten >= 0);
		object.push_back(makeFeature("width",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%f",meanGrayValue);
		assert(charWritten >= 0);
		object.push_back(makeFeature("mean_gray",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%u",tl_x);
		assert(charWritten >= 0);
		object.push_back(makeFeature("tl_x",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%u",tl_y);
		assert(charWritten >= 0);
		object.push_back(makeFeature("tl_y",tempData,charWritten));

		charWritten = sprintf(tempData,"%u",br_x);
		assert(charWritten >= 0);
		object.push_back(makeFeature("br_x",tempData,charWritten));

		charWritten = sprintf(tempData,"%u",br_y);
		assert(charWritten >= 0);
		object.push_back(makeFeature("br_y",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%f",cent_x);
		assert(charWritten >= 0);
		object.push_back(makeFeature("cent_x",tempData,charWritten));
		
		charWritten = sprintf(tempData,"%f",cent_y);
		assert(charWritten >= 0);
		object.push_back(makeFeature("cent_y",tempData,charWritten));

		charWritten = sprintf(tempData,"%f",personProbability);
		assert(charWritten >= 0);
		object.push_back(makeFeature("person_probability",tempData,charWritten));

		charWritten = sprintf(tempData,"%f",carProbability);
		assert(charWritten >= 0);
		object.push_back(makeFeature("car_probability",tempData,charWritten));
		
		//END Populate features
		objects.push_back(object);
	}

	sprintf(tempName, "output/finaldiff/%.6u.png",frameCounter);
	devil->writeImage(*diffImage, tempName);
	if (objects.size() > 0)
	{
		if (!printedHeader)
		{
			printedHeader = true;
			for (unsigned int featureIter = 0; featureIter < objects[0].size(); featureIter++)
				fprintf(featureFile,"%s ",objects[0][featureIter][0].c_str());
			fprintf(featureFile,"\n");
		}
		fprintf(featureFile,"Frame %u\n",frameCounter);
		for (unsigned int objectIter = 0; objectIter < objects.size(); objectIter++)
		{
			for (unsigned int featureIter = 0; featureIter < objects[0].size(); featureIter++)
			{
				fprintf(featureFile,"%s ",objects[objectIter][featureIter][1].c_str());
			}
			fprintf(featureFile,"\n");
		}
		fflush(featureFile);
	}

	
	frameCounter++;
	return objects;
}

