#include "imageProvider/ListLoader.h"
#include "imageProvider/UDPClient.h"
#include <list>
#include <cassert>
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "BackgroundSubtraction.h"
#include "backgroundSubtraction/kgaussgrimson.h"
#include "backgroundSubtraction/kgausspower.h"
#include "imageTools/connectedComponents.h"
//#include "backgroundSubtraction/kgaussiansubtraction.h"
#include "backgroundSubtraction/NonParametric.h"
using namespace std;

int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	
	Devil devil;
	UDPClient* imProvide = new UDPClient(configList);
	
	unsigned int minObjectArea;
	unsigned int notLoaded = 0;
	notLoaded += FindValue(configList, "MinObjectArea", minObjectArea);
	assert(!notLoaded);
	
	
	unsigned int height, width, channels;
	imProvide->getImageInfo(height, width, channels);	
	Image8 diffImage(height,width,1);
	Image8 currentImage(height,width,3);
	char tempName[100];
	//BackgroundSubtraction* bgSub = new KGaussianPower(height,width,channels,imProvide,configList);
	BackgroundSubtraction* bgSub = new KGaussianGrimson(height,width,channels,imProvide,configList);
	//BackgroundSubtraction* bgSub = new NonParametric(height,width,channels,imProvide,configList);
	//BackgroundSubtraction* bgSub = new KGaussianSubtraction(height,width,channels,imProvide,configList);
	unsigned int curFrame = 0;
	for (;;)
	{
		cout << "Image " << curFrame << endl;
		//cout << "Image" << imProvide->getFrameCount() << endl;
		
		bgSub->subtract(diffImage,currentImage);
		//TODO Change to pgm if lossless is required
		//sprintf(tempName, "./output/out%.5u.pgm", curFrame++);
		//devil.writeImage(diffImage, tempName);
		
		if (minObjectArea > 0)
		{
			//Remove noise
			list<list<connectedPoint_t> > objectList;
			getConnectedObjects(diffImage, objectList);
			for (list<list<connectedPoint_t> >::iterator objIter = objectList.begin(); objIter != objectList.end(); objIter++)
				if (objIter->size() < minObjectArea)
					for (list<connectedPoint_t>::iterator pixIter = objIter->begin(); pixIter != objIter->end(); pixIter++)
						diffImage(pixIter->y, pixIter->x) = 0;
		}
		
		//TODO Change to pgm if lossless is required
		sprintf(tempName, "./output/ccout%.5u.pgm", curFrame);
		devil.writeImage(diffImage, tempName);
		
		//TODO Do simple connected components and apply boxes to output images
		sprintf(tempName, "./output/current%.5u.ppm", curFrame);
		devil.writeImage(currentImage, tempName);
		curFrame++;
	}
	
	delete bgSub;
	delete imProvide;
	return 0;
}
