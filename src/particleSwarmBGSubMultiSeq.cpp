/*
This is a version of the PSO main file that can run multiple sequences.
Main Differences...
Frames for each sequence are listed as such
								Frame\Frame\...
PSOFrames1                      100\300\500\3323

								Ground Truth Location\File List
PSOSeq7                         ../input/MovedObject/truth/\MovedObject.list
*/

#include "imageProvider/ListLoader.h"
#include <list>
#include <vector>
#include <cassert>
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "BackgroundSubtraction.h"
#include "backgroundSubtraction/kgaussgrimson.h"
//#include "backgroundSubtraction/kgausspower.h"
//#include "backgroundSubtraction/kgaussiansubtraction.h"
//#include "backgroundSubtraction/NonParametric.h"
#include "PSO/particleSwarm.h"
#include "PSO/fitness.h"
#include "imageTools/connectedComponents.h"
#include "backgroundSubtraction/NonParametric.h"
#include "backgroundSubtraction/NonParametricGPU.h"
using namespace std;


int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	list<ImageFormat*> formats;

	char tempIterBuffer[100];
	char tempDataBuffer[100];
	std::vector<psoSeq_t> psoSeqs;
	for (unsigned int i= 0; i <= 20; i++)
	{
		sprintf(tempIterBuffer,"PSOSeq%d",i);
		if (!FindValue(configList, tempIterBuffer, tempDataBuffer))
		{
			psoSeq_t temp;
			strcpy(temp.truthPath, strtok(tempDataBuffer,"\\"));
			strcpy(temp.fileList, strtok(0,"\\"));
			psoSeqs.push_back(temp);
		}
	}

	formats.push_back(new Devil);

	vector<ImageProvider*> imProviders;
	unsigned int height = 0, width = 0, channels = 0;
	unsigned int startFrame;
	for (unsigned int fitIter = 0; fitIter < psoSeqs.size(); fitIter++)
	{
		imProviders.push_back(new ListLoader(configList, formats, psoSeqs[fitIter].fileList));
	
		unsigned int tempHeight, tempWidth, tempChannels;
		imProviders[fitIter]->getImageInfo(tempHeight, tempWidth, tempChannels);
		if (fitIter)
		{
			assert(tempHeight == height);
			assert(tempWidth == width);
			assert(tempChannels == channels);
		}
		height = tempHeight;
		width = tempWidth;
		channels = tempChannels;
	}

	unsigned int notLoaded = 0;
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
	for (unsigned int i= 0; i <= 20; i++)//TODO Change this so that we say how many dimensions to look for, than we iterate to that
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
	
	vector<Fitness*> fitnessVec;
	for (unsigned int fitIter = 0; fitIter < psoSeqs.size(); fitIter++)
	{
		sprintf(tempIterBuffer,"PSOFrames%d",fitIter);
		notLoaded += FindValue(configList, tempIterBuffer, tempDataBuffer);
		assert(!notLoaded);
		Fitness *temp = new Fitness(height,width, psoSeqs[fitIter].truthPath, tempDataBuffer, configList);
		fitnessVec.push_back(temp);
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
			double totalFitness = 0;
			unsigned int truthIter = 0;
			agentIter->updateVelocityPosition();	
			agentIter->saveToConfig(configList);
			fprintf(logFile, "%u %u ",b, agentID);
			agentIter->saveToFile(logFile);
			
			unsigned int sequences = 0;
			for (unsigned int imProvIter = 0; imProvIter  < psoSeqs.size(); imProvIter++)
			{
				sequences++;
				imProviders[imProvIter]->reset();
				//BackgroundSubtraction* bgSub = new KGaussianPower(height,width,channels,imProviders[imProvIter],configList);
				BackgroundSubtraction* bgSub = new NonParametricGPU(height,width,channels,imProviders[imProvIter],configList);
				//BackgroundSubtraction* bgSub = new KGaussianGrimson(height,width,channels,imProviders[imProvIter],configList);
				//BackgroundSubtraction* bgSub = new NonParametric(height,width,channels,imProvide,configList);
				//BackgroundSubtraction* bgSub = new KGaussianSubtraction(height,width,channels,imProvide,configList);
				
				imProviders[imProvIter]->reset();
				
				double sequenceFitness = 0;
				unsigned int sequenceTruthFrames = 0;
				for (unsigned int i = startFrame; i <= fitnessVec[imProvIter]->getLastTruthFrame(); i++)
				{
					//cout << "\nPSO-Frame: " << i << endl;
					bgSub->subtract(diffImage,currentImage);
					//sprintf(tempName, "../tempoutput/out%.2u.jpg", i);
					//jpeg.writeImage(diffImage, tempName);
					if (fitnessVec[imProvIter]->hasTruthFrame(i))
					{
						sequenceTruthFrames++;
						fprintf(statisticFile, "%u %u %u ",b, agentID,truthIter++);
						
						if (minObjectArea > 0)
						{
							//Remove noise
							list<list<connectedPoint_t> > objectList;
							getConnectedObjects(diffImage, objectList);
							for (list<list<connectedPoint_t> >::iterator objIter = objectList.begin(); objIter != objectList.end(); objIter++)
								if (objIter->size() < minObjectArea)
									for (list<connectedPoint_t>::iterator pixIter = objIter->begin(); pixIter != objIter->end(); pixIter++)
										diffImage.write(pixIter->y, pixIter->x,0,0);
						}
									
						sequenceFitness += fitnessVec[imProvIter]->computeFrameFitness(diffImage,i, statisticFile);
						fflush(statisticFile);
					}
				}
				totalFitness += sequenceFitness / (double) sequenceTruthFrames;
				delete bgSub;
			}
			agentID++;
			totalFitness = totalFitness / (double)sequences;
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
	return 0;
}
