#include "imageProvider/ListLoader.h"
#include "imageProvider/OpenCVReader.h"
#include <list>
#include <cassert>
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "BackgroundSubtraction.h"
#include "backgroundSubtraction/kgaussgrimson.h"
#include "backgroundSubtraction/kgausspower.h"
#include "imageTools/connectedComponents.h"
#include "backgroundSubtraction/NonParametricGPU.h"
//#include "backgroundSubtraction/kgaussiansubtraction.h"
#include "backgroundSubtraction/NonParametric.h"

using namespace std;

int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);

	OpenCVReader* imProvide = new OpenCVReader(configList);
	
	unsigned int height, width, channels;
	imProvide->getImageInfo(height, width, channels);	
	Image8 diffImage(height,width,1);
	Image8 currentImage(height,width,3);
	char tempName[100];
	//BackgroundSubtraction* bgSub = new NonParametricGPU(height,width,channels,imProvide,configList);
	BackgroundSubtraction* bgSub = new KGaussianGrimson(height,width,channels,imProvide,configList);
	//BackgroundSubtraction* bgSub = new KGaussianPower(height,width,channels,imProvide,configList);
	
	Devil devil;
	unsigned int curFrame = 0;
	while (1)
	{
		if (curFrame % 10 == 0)
			cout << "Frame - " << curFrame << '\n';
		bgSub->subtract(diffImage,currentImage);
		//TODO Change to pgm if lossless is required
		sprintf(tempName, "./output/cur%.5u.png", curFrame);
		devil.writeImage(currentImage, tempName);
		
		sprintf(tempName, "./output/out%.5u.png", curFrame);
		devil.writeImage(diffImage, tempName);

		//CCScanline
		/*if (minObjectArea > 0)
		{
			//Remove noise
			list<list<connectedPoint_t> > objectList;
			getConnectedObjects(diffImage, objectList);
			for (list<list<connectedPoint_t> >::iterator objIter = objectList.begin(); objIter != objectList.end(); objIter++)
				if (objIter->size() < minObjectArea)
				{
					for (list<connectedPoint_t>::iterator pixIter = objIter->begin(); pixIter != objIter->end(); pixIter++)
						diffImage.write(pixIter->y, pixIter->x,0,0);
				}
		}*/
		
		//TODO Change to pgm if lossless is required
		//sprintf(tempName, "./output/ccout%.5u.png", curFrame);
		//devil.writeImage(diffImage, tempName);
		curFrame++;
	}
	
	delete bgSub;
	delete imProvide;
	return 0;
}
