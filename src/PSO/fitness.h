#ifndef FITNESS_H
#define FITNESS_H
#include "../imageFormat/devil.h"
#include "../configTools/configReader.h"
#include <list>
#include <vector>
#include <cassert>

using namespace std;
typedef struct
{
	unsigned int frame;
	Image8 *image;
} truthFrame_t;

typedef struct
{
	char truthPath[100];
	char fileList[100];
} psoSeq_t;

class Fitness
{
private:
	//TODO
	//Data structure that holds truth images and frame numbers
	char truthExtension[20];
	list<truthFrame_t> frames;
	const unsigned int height;
	const unsigned int width;
public:
	//Frames string is the string in the format of 32\342\32\4543
	Fitness(unsigned int height, unsigned int width, char* truthPath, char *framesString,std::list<vector<string> > &configList);	
	~Fitness();
	bool hasTruthFrame(unsigned int frameNumber) const;
	Image8* getTruthFrame(unsigned int frameNumber);	
	///Returns the largest truthFrame number
	unsigned int getLastTruthFrame();
	///This will take the given one channel input image, and compute the fscore for it based on a given truth image
	double computeFrameFitness(Image8 &difference, unsigned int frameNumber, FILE *file = 0);
};
#endif
