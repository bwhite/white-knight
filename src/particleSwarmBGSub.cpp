#include "imageProvider/ListLoader.h"
#include <list>
#include <cassert>
#include "configTools/configReader.h"
//#include "imageFormat/jpg.h"
#include "imageFormat/devil.h"
#include "BackgroundSubtraction.h"
#include "backgroundSubtraction/kgaussgrimson.h"
//#include "backgroundSubtraction/kgausspower.h"
//#include "backgroundSubtraction/kgaussiansubtraction.h"
#include "PSO/particleSwarm.h"
#include "PSO/fitness.h"
#include "imageTools/connectedComponents.h"
#include "backgroundSubtraction/NonParametric.h"
//#include "backgroundSubtraction/NonParametricGPU.h"
#include "imageTools/imageTools.h"

using namespace std;


int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	list<ImageFormat*> formats;

	formats.push_back(new Devil);

	unsigned int notLoaded = 0;
	
	char tempIterBuffer[100];
	char tempDataBuffer[100];
	psoSeq_t singleSeq;
	sprintf(tempIterBuffer,"PSOSeq%d",0);
	notLoaded += FindValue(configList, tempIterBuffer, tempDataBuffer);
	strcpy(singleSeq.truthPath, strtok(tempDataBuffer,"\\"));
	strcpy(singleSeq.fileList, strtok(0,"\\"));
	
	sprintf(tempIterBuffer,"PSOFrames%d",0);
	notLoaded += FindValue(configList, tempIterBuffer, tempDataBuffer);
	
	assert(!notLoaded);
	
	ImageProvider* imProvide = new ListLoader(configList, formats, singleSeq.fileList);
	
	unsigned int height, width, channels;
	unsigned int startFrame;
	imProvide->getImageInfo(height, width, channels);
	
	Fitness fitness(height,width, singleSeq.truthPath, tempDataBuffer, configList);
	
	notLoaded += FindValue(configList, "StartFrame", startFrame);
	assert(!notLoaded);
	
	//This is where we create the particle swarm 'agents', and the dimensions that they contain
	unsigned int numAgents, numIters;
	notLoaded += FindValue(configList, "PSONumAgents", numAgents);
	notLoaded += FindValue(configList, "PSONumIters", numIters);
	assert(!notLoaded);
	
	//Gather limits for each dimension for PSO
	//The format is parsed with strok, the delimeter being / and example is...
	//MinSqrMahalDistanceHighPass/60/100		or	Name/Minimum/Maximum
	std::list<swarmDimension_t> dimensions;
	for (unsigned int i= 1; i <= 20; i++)//TODO Change this so that we say how many dimensions to look for, than we iterate to that
	{
		sprintf(tempIterBuffer,"PSODimension%d",i);
		if (!FindValue(configList, tempIterBuffer, tempDataBuffer))
		{
			swarmDimension_t temp;
			strcpy(temp.name, strtok(tempDataBuffer,"/"));
			sscanf(strtok(0,"/"),"%lf",&temp.minValue);
			sscanf(strtok(0,"/"),"%lf",&temp.maxValue);
			dimensions.push_back(temp);
		}
	}

	//Create list of agents for particle swarming
	std::list<ParticleSwarmAgent> agents;
	for (unsigned int m = 0; m < numAgents; m++)
	{
		ParticleSwarmAgent tempAgent(dimensions, configList);
		agents.push_back(tempAgent);
	}
	
	//Load minimum object area
	unsigned int minObjectArea;
	notLoaded += FindValue(configList, "MinObjectArea", minObjectArea);
	assert(!notLoaded);
	
	//Setup log files
	unsigned int runNum;
	notLoaded += FindValue(configList, "RunNumber", runNum);
	assert(!notLoaded);
	char tempLogNameBuffer[100];
	sprintf(tempLogNameBuffer,"Run%.2u-stats.log",runNum);
	FILE* statisticFile = fopen(tempLogNameBuffer,"w");
	sprintf(tempLogNameBuffer,"Run%.2u-log.log",runNum);
	FILE* logFile = fopen(tempLogNameBuffer,"w");
	
	//Load global illumination detection vector
	char ilumFilename[100];
	vector<double> illuminationProbabilities;//For the global illumination probabilities, always starts at first frame
	double globalIlluminationThreshold;

	notLoaded += FindValue(configList, "FileList", ilumFilename);
	assert(!notLoaded);
	
	int boolConvert;
        notLoaded += FindValue(configList, "ChangeDetectionEnable", boolConvert);
        bool enableIlluminationDetect = boolConvert;

	if (enableIlluminationDetect)
	{
		strcat(ilumFilename,".ilum");	
		FILE *ilumFile = fopen(ilumFilename,"r+");
	
		if (!ilumFile)
		{
			cerr << "Couldn't open illumination file!" << endl;
		}
	
		while (!feof(ilumFile))
		{
			double tempDouble;
			fscanf(ilumFile,"%lf ",&tempDouble);
			illuminationProbabilities.push_back(tempDouble);
		}
		fclose(ilumFile);
	}
	
	Image8 diffImage(height,width,1);
	Image8 currentImage(height,width,3);
	fprintf(logFile, "Generations Agent ");
	agents.begin()->saveHeaderToFile(logFile);
	fprintf(logFile, "Fitness\n");
	for (unsigned int b = 0; b < numIters; b++)//Generations
	{
		cout << "Generation: " << b << endl;
		unsigned int agentID = 0;
		double generationFitnessSum = 0;
		for (list<ParticleSwarmAgent>::iterator agentIter = agents.begin(); agentIter != agents.end(); agentIter++)
		{
			imProvide->reset();
			
			double totalFitness = 0;
			unsigned int truthIter = 0;
			cout << fitness.getLastTruthFrame() << endl;
			
			agentIter->updateVelocityPosition();
			
			agentIter->saveToConfig(configList);
			fprintf(logFile, "%u %u ",b, agentID);
			agentIter->saveToFile(logFile);
			
			unsigned int minObjectArea, blurIters, blurThreshold;
			notLoaded += FindValue(configList, "MinObjectArea", minObjectArea);
			notLoaded += FindValue(configList, "BlurIters", blurIters);
			notLoaded += FindValue(configList, "BlurThreshold", blurThreshold);
			notLoaded += FindValue(configList, "GlobalIlluminationThreshold", globalIlluminationThreshold);
			assert(!notLoaded);
			
			//BackgroundSubtraction* bgSub = new KGaussianPower(height,width,channels,imProvide,configList);
			//BackgroundSubtraction* bgSub = new NonParametricGPU(height,width,channels,imProvide,configList);
			//BackgroundSubtraction* bgSub = new KGaussianGrimson(height,width,channels,imProvide,configList);
			BackgroundSubtraction* bgSub = new NonParametric(height,width,channels,imProvide,configList);
			//BackgroundSubtraction* bgSub = new KGaussianSubtraction(height,width,channels,imProvide,configList);


			unsigned int numTruthFrames = 0;
			for (unsigned int i = startFrame; i <= fitness.getLastTruthFrame(); i++)
			{
				if (i % 10 == 0)
					cout << "\nPSO-Frame: " << i << endl;
				if (enableIlluminationDetect && i < illuminationProbabilities.size() && illuminationProbabilities[i] > globalIlluminationThreshold)
					bgSub->hint("GlobalIlluminationChange", 1.0);
				bgSub->subtract(diffImage, currentImage);
				//sprintf(tempName, "../tempoutput/out%.2u.jpg", i);
				//jpeg.writeImage(diffImage, tempName);
				if (fitness.hasTruthFrame(i))
				{
					numTruthFrames++;
					fprintf(statisticFile, "%u %u %u ",b, agentID,truthIter++);
				/*	
					for (unsigned int blurIter = 0; blurIter < blurIters; blurIter++)
						skipsmGaussBlur5(diffImage, diffImage);
					
					//Clear a 2 pixels on every edge (as the blur doesn't place any new values there)
					for (unsigned int yRemovePixIter = 0; yRemovePixIter < diffImage.getHeight(); yRemovePixIter++)
					{
						for (unsigned int xRemovePixIter = 0; xRemovePixIter < 2; xRemovePixIter++)
							diffImage.write(yRemovePixIter,xRemovePixIter,0,0);
						for (unsigned int xRemovePixIter = diffImage.getWidth()-2; xRemovePixIter < diffImage.getWidth(); xRemovePixIter++)
							diffImage.write(yRemovePixIter,xRemovePixIter,0,0);
					}
					for (unsigned int xRemovePixIter = 0; xRemovePixIter < diffImage.getWidth(); xRemovePixIter++)
					{
						for (unsigned int yRemovePixIter = 0; yRemovePixIter < 2; yRemovePixIter++)
							diffImage.write(yRemovePixIter,xRemovePixIter,0,0);
						for (unsigned int yRemovePixIter = diffImage.getHeight()-2; yRemovePixIter < diffImage.getHeight(); yRemovePixIter++)
							diffImage.write(yRemovePixIter,xRemovePixIter,0,0);
					}
					
					for (unsigned int yRemovePixIter = 0; yRemovePixIter < diffImage.getHeight(); yRemovePixIter++)
						for (unsigned int xRemovePixIter = 0; xRemovePixIter < diffImage.getWidth(); xRemovePixIter++)
						{
							if (yRemovePixIter >= diffImage.getHeight()-2 || xRemovePixIter >= diffImage.getWidth()-2 || yRemovePixIter < 2 || xRemovePixIter < 2)
								diffImage.write(yRemovePixIter,xRemovePixIter,0,0);
						}
					
					thresholdImage(diffImage, blurThreshold);//TODO all of these constants need to be in the config file!
					*/
					/*if (minObjectArea > 0)
					{
						//Remove noise
						list<list<connectedPoint_t> > objectList;
						getConnectedObjects(diffImage, objectList);
						for (list<list<connectedPoint_t> >::iterator objIter = objectList.begin(); objIter != objectList.end(); objIter++)
							if (objIter->size() < minObjectArea)
								for (list<connectedPoint_t>::iterator pixIter = objIter->begin(); pixIter != objIter->end(); pixIter++)
									diffImage.write(pixIter->y, pixIter->x,0,0);
					}*/
						
						totalFitness += fitness.computeFrameFitness(diffImage,i, statisticFile);
						fflush(statisticFile);
				}
			}
			agentID++;
			delete bgSub;
			totalFitness = totalFitness/(double)numTruthFrames;
			cout << "Fitness: " << totalFitness << endl;
			fprintf(logFile, " %f\n",totalFitness);
			agentIter->updateMaxima(totalFitness);
			fflush(statisticFile);
			fflush(logFile);
			generationFitnessSum += totalFitness;
		}
	}

	fclose(statisticFile);
	fclose(logFile);
	delete imProvide;
	return 0;
}
