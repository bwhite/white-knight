#ifndef IMGTOOLS_H
#define IMGTOOLS_H
#include "greyimage8.h"
#include "colorimage8.h"
#include <list>
#include "config_reader.h"
#include <iostream>
typedef struct
{
	unsigned int br_x;//These form the box around the image
	unsigned int br_y;
	unsigned int tl_x;
	unsigned int tl_y;
	unsigned int area;
	unsigned int width;
	unsigned int height;
	double meanColor;
	unsigned int edge_size;
	double center_x;
	double center_y;
	unsigned int seed_x;//Seed x/y are used when forcing background as they correspond to points on the object (tl/br/center aren't necessarily on the object)
	unsigned int seed_y;
	unsigned int nodeNum;
	bool matchedThisFrame;//Used by the tracker to signify that we have used it
}
connectedObject_t;

void fillBox(GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char color);
void RemoveShadow(const ColorImage8 &currentImage, const ColorImage8 &backgroundImage, const GreyImage8 &differenceImage, GreyImage8 &deshadowedImage,std::list<configElement_t> configList, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void SubImage (const ColorImage8 &inputImage, ColorImage8 &subImage, unsigned int tl_x, unsigned int tl_y);
void SubImage (const GreyImage8 &inputImage, GreyImage8 &subImage, unsigned int tl_x, unsigned int tl_y);
void RGB2Grey(const ColorImage8 &inputImage, GreyImage8 &outputImage);
void Grey2RGB(const GreyImage8 &inputImage, ColorImage8 &outputImage);
void RGB2HSV(unsigned char r, unsigned char g, unsigned char b, double &h, double &s, double &v);
void calculateSearchSpace(GreyImage8 &inputImage, unsigned int &ssTop, unsigned int &ssBottom, unsigned int &ssLeft, unsigned int &ssRight);
void Dilate(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Erode(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Erode(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage);
void Dilate(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage);
void joinComponents(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void connectedComponenets(GreyImage8 &differenceImage, GreyImage8 &tempImage, const GreyImage8 &edgeDifference, const GreyImage8 &inputImage, std::list<connectedObject_t> &objectList, unsigned int minArea,unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void drawBox (ColorImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char red, unsigned char green, unsigned char blue);
void drawBox (GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned int color);
void drawBox (ColorImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y);
void drawBox (GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y);
void Sobel(const GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Subtract(const GreyImage8 &inputImage1, const GreyImage8 &inputImage2, GreyImage8 &outputImage);
void Subtract(const GreyImage8 &inputImage1, const GreyImage8 &inputImage2, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void getGaussianKernel(double *&kernel, double sigma, unsigned int radius);
void GaussianBlur(const GreyImage8 &inputImage, GreyImage8 &outputImage, double *&kernel, unsigned int radius, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void ThresholdGrey(GreyImage8 &inputImage, unsigned char threshold);
void GaussianBlur3(GreyImage8 &inputImage, GreyImage8 &outputImage);
void GaussianBlur5(GreyImage8 &inputImage, GreyImage8 &outputImage);
#endif
