#include <iostream>
#include <ctime>
#include <vector>
#include "../objectClassifier/SVMClassifier.h"
#include "../configTools/configReader.h"
#include "../imageFormat/devil.h"
#include "../imageProvider/ChipLoader.h"
#include "../imageTools/imageTools.h"
#include <opencv/cv.h>
#include <cmath>

//'output' should have its memory allocated
void Image8ToOpenCV(const Image8 &input, IplImage* output);

/*This takes takes a list of chips that are all of the same target size and gray and output a model file
*/

void GradientMagnitudeDirection(const Image8 &inputImage, std::vector<double> &featureVector);

void heightWidthRatio(const Image8 &inputImage, std::vector<double> &featureVector);

//From paper "Histograms of Oriented Gradients for Human Detection"
void histogramOrientedGradients(const Image8 &inputImage, std::vector<double> &featureVector);

//From paper "Human Detection Using Oriented Histograms of Flow and Appearance"
void histogramOrientedFlow(const Image8 &inputImage1, const Image8 &inputImage2,std::vector<double> &featureVector);
void featureCalculation(vector<double> &currentVector, unsigned int featureMode, Image8 *currentChip, Image8* currentChipTwin = 0);