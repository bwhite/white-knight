#ifndef CONCOMP_H
#define CONCOMP_H
#include <list>
#include <vector>
#include "../imageClass/Image8.h"
using namespace std;

//A connected object is essentially a list of (x,y) coordinates on an image that all belong to one contiguous object
//This is intentionally simple so that if, for example, you want to find the average gray value for all of the connected objects
//you can easily take each connected object and find this for each of its points
typedef struct
{
	unsigned int x;
	unsigned int y;
} connectedPoint_t;
void getConnectedObjects(Image8 &image, list<list<connectedPoint_t> > &connectedObjects, bool scanline = true);
#endif
