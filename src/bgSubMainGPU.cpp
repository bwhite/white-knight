#include "imageProvider/ListLoader.h"
#include <list>
#include <cassert>
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "BackgroundSubtraction.h"
#include "backgroundSubtraction/kgaussgrimsongpu.h"
#include "backgroundSubtraction/NonParametricGPU.h"
#include "imageTools/connectedComponents.h"
using namespace std;

int main(int argc, char **argv)
{
	assert(argc == 2);
	list<vector<string> > configList;
	ReadConfig(argv[1], configList);
	list<ImageFormat*> formats;
	
	formats.push_back(new Devil);
	ListLoader* imProvide = new ListLoader(configList, formats);
	
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
	BackgroundSubtraction* bgSub = new NonParametricGPU(height,width,channels,imProvide,configList);
	//BackgroundSubtraction* bgSub = new NonParametric(height,width,channels,imProvide,configList);
	//BackgroundSubtraction* bgSub = new KGaussianSubtraction(height,width,channels,imProvide,configList);
	Devil devil;
	while (imProvide->getNumImagesRemaining() != 0)
	{
		//cout << "Image" << imProvide->getFrameCount() << endl;
		unsigned int curFrame = imProvide->getFrameCount();
		bgSub->subtract(diffImage,currentImage);
		//TODO Change to pgm if lossless is required
		sprintf(tempName, "./output/out%.5u.pgm", curFrame);
		devil.writeImage(diffImage, tempName);
		
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
		
		//TODO Change to pgm if lossless is required
		sprintf(tempName, "./output/ccout%.5u.pgm", curFrame);
		devil.writeImage(diffImage, tempName);
	}
	
	delete bgSub;
	delete imProvide;
	return 0;
}
