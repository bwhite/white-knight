#ifndef __CHANGEDETECTOR__
#define __CHANGEDETECTOR__

#include "../configTools/configReader.h"
#include "../ImageProvider.h"

#include <vector>

using namespace std;

class ChangeDetector
{
public:
	ChangeDetector(list<vector<string> > configList, ImageProvider *imProvider);
	
	// The elements from 0 to 'window'/2 will be zero. 
	void detectChanges(vector<double> &changeScore);
	void saveVector(const char* fileName);
	void loadVector(const char* fileName);
protected:
	unsigned int window;
	unsigned int end;
	unsigned int height;
	unsigned int width;
	bool limitedRange;//True if we impose a window, false if unlimited
	ImageProvider *imProvider;
};
#endif
