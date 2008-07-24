#include "imageProvider/ListLoader.h"
#include <list>
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
using namespace std;

int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	list<ImageFormat*> formats;

	char tempIterBuffer[100];
	char tempDataBuffer[100];
	psoSeq_t singleSeq;
	unsigned int notLoaded = 0;
	sprintf(tempIterBuffer,"PSOSeq%d",0);
	notLoaded += FindValue(configList, tempIterBuffer, tempDataBuffer);
	strcpy(singleSeq.truthPath, strtok(tempDataBuffer,"\\"));
	strcpy(singleSeq.fileList, strtok(0,"\\"));
	
	sprintf(tempIterBuffer,"PSOFrames%d",0);
	notLoaded += FindValue(configList, tempIterBuffer, tempDataBuffer);
	
	assert(!notLoaded);

	formats.push_back(new Devil);
	ImageProvider* imProvide = new ListLoader(configList, formats, singleSeq.fileList);

	unsigned int height, width, channels;
	unsigned int startFrame;
	imProvide->getImageInfo(height, width, channels);

	notLoaded += FindValue(configList, "StartFrame", startFrame);
	assert(!notLoaded);
		
	Fitness fitness(height,width, singleSeq.truthPath, tempDataBuffer, configList);

	Image8 diffImage(height,width,1);
	Image8 currentImage(height,width,3);
	
	//Load minimum object area
	unsigned int minObjectArea;
	notLoaded += FindValue(configList, "MinObjectArea", minObjectArea);
	assert(!notLoaded);
	
	//Setup log files
	unsigned int runNum;
	notLoaded += FindValue(configList, "RunNumber", runNum);
	assert(!notLoaded);
	char tempLogNameBuffer[100];
	sprintf(tempLogNameBuffer,"Run%.2u-uniform.log",runNum);
	FILE* uniformFile = fopen(tempLogNameBuffer,"w");
	sprintf(tempLogNameBuffer,"Run%.2u-matrix.log",runNum);
	FILE* matrix = fopen(tempLogNameBuffer,"w");
	
	//Set alpha to .5
	double paramInc1 = .005;
	double minParam1 = 0;
	double maxParam1 = .52;
	/*double minParam1 = .04;
	double maxParam1 = .3;
	double paramInc1 = .005;*/\
	char paramName1[] = "Alpha";
	double paramInc2 = .05;
	double minParam2 = 0.0;
	double maxParam2 = .6;
	char paramName2[] = "MinTotalBgWeight";

	fprintf(uniformFile,"surf(%f:%f:%f,%f:%f:%f,matrix);\n",minParam2, paramInc2, maxParam2, minParam1, paramInc1, maxParam1);

	//TODO Just for-loop here for number of random selections
	//The addition of half an increment is to solve the floating point errors accumulated with the <=
	for (double param1 = minParam1; param1 <= maxParam1 + (paramInc1/2.0); param1 += paramInc1)//Alpha
	{
		for (double param2 = minParam2; param2 <= maxParam2 + (paramInc2/2.0); param2 += paramInc2)//T - Prior background probability
		{
			imProvide->reset();
			double totalFitness = 0;

			int returnVal = 0;
			returnVal += ForceValue(configList, paramName1, param1);
			returnVal += ForceValue(configList, paramName2, param2);
			assert(!returnVal);
			
			char tempName[100];
			//BackgroundSubtraction* bgSub = new KGaussianPower(height,width,channels,imProvide,configList);
			BackgroundSubtraction* bgSub = new KGaussianGrimson(height,width,channels,imProvide,configList);
			//BackgroundSubtraction* bgSub = new NonParametric(height,width,channels,imProvide,configList);
			//BackgroundSubtraction* bgSub = new KGaussianSubtraction(height,width,channels,imProvide,configList);


			unsigned int fitnessFrames = 0;
			for (unsigned int i = startFrame; i <= fitness.getLastTruthFrame(); i++)
			{
				//cout << "\nPSO-Frame: " << i << endl;
				bgSub->subtract(diffImage,currentImage);
				sprintf(tempName, "./output/out%.2u.jpg", i);
				//jpeg.writeImage(diffImage, tempName);
				if (fitness.hasTruthFrame(i))
				{
					fitnessFrames++;
					//Go through objects, if an object is less than some area, we set its pixels to 0
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
					totalFitness += fitness.computeFrameFitness(diffImage,i);
				}
			}
			delete bgSub;
			totalFitness = totalFitness / (double)fitnessFrames;
			fprintf(uniformFile, "%f %f %f\n", param1, param2, totalFitness);
			fprintf(matrix, "%f ",totalFitness);
			cout << "Fitness: " << totalFitness << endl;
			fflush(uniformFile);
			fflush(matrix);
		}
		fprintf(matrix, "\n");
	}

	fclose(matrix);
	fclose(uniformFile);
	delete imProvide;
	return 0;
}
