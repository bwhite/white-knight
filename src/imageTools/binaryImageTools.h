#ifndef _BINARYIMAGETOOLS_H_
#define _BINARYIMAGETOOLS_H_
#include "../imageClass/Image8.h"
void joinComponents(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Dilate(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Erode(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);

#endif /* _BINARYIMAGETOOLS_H_ */
