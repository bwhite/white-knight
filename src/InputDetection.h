#ifndef _INPUTDETECTION_H_
#define _INPUTDETECTION_H_
#ifdef SWIG
%include "std_vector.i"
%include "std_list.i"
%include "std_string.i"

%module InputDetection
%{
#include <vector>
#include <list>
#include <iostream>
#include "src/InputDetection.h"
#include "src/configTools/configReader.h"
#include "src/imageTools/font.h"
using namespace std;
%}
namespace std
{
	%template(Dim1VecStr) vector<string>;
	%template(Dim2VecStr) vector<vector<string> >;
	%template(Dim3VecStr) vector<vector<vector<string> > >;
	%template(ListDim1VecStr) list<vector<string> >;
	%template(VecUInt) vector<unsigned int>;
	%template(ListVecUInt) list<vector<unsigned int> >;
}
#endif

#include "BackgroundSubtraction.h"
#include "imageProvider/ListLoader.h"
//#include "imageProvider/UDPClient.h"
#include <list>
#include <cassert>
#include "configTools/configReader.h"
#include "imageFormat/devil.h"
#include "imageTools/font.h"
#include "objectClassifier/SVMClassifier.h"
#include "imageTools/imageTools.h"

using namespace std;

///This is a wrapper for both the input and detection modules, its primary purpose is to allow for use in Python.
class InputDetection
{
public:
	InputDetection(string configPath);
	InputDetection(list<vector<string> > configList);
	~InputDetection();
	vector<vector<vector<string> > > getObjects() throw (NoMoreImages);//Throws errors if no more images can be aquired
	void outputData(list<vector<unsigned int> > box_data);
	unsigned int getHeight()
	{
		return currentImage->getHeight();
	}

	unsigned int getWidth()
	{
		return currentImage->getWidth();
	}
protected:
	void setup(list<vector<string> > configList);
	ImageProvider *imProvide;
	BackgroundSubtraction *bgSub;
	Image8 *diffImage;
	Image8 *tempMorphologicalImage;
	Image8 *tempGrayImage;
	Image8 *fgEdge;
	Image8 *bgEdge;
	Image8 *currentImage;
	Image8 *tempColorImage;
	Image8 *forceBGMask;
	Devil *devil;
	FILE *featureFile;
	unsigned int frameCounter;
	bool printedHeader;
	Image8 *boxImage;
	unsigned int blurIters;
	unsigned int blurThreshold;
	unsigned int joinComponentsX;
	unsigned int joinComponentsY;
	unsigned int minObjectArea;
	double fgEdgeThresh;
	double fgEdgeRatioThresh;

	//SVM Model holders
	SVMClassifier svmPerson;
	SVMClassifier svmCar;
	Image8 svmChip;
	
	vector<double> illuminationProbabilities;//For the global illumination probabilities, always starts at first frame
	double globalIlluminationThreshold;
	Font* font;
};

#endif /* _INPUTDETECTION_H_ */
