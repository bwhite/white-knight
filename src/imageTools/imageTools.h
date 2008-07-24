#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H
#include "../imageClass/Image8.h"
void BilinearResample(Image8 &input, Image8 &output);
void Gray2RGB(const Image8 &inputImage, Image8 &outputImage);
void RGB2Gray(const Image8 &inputImage, Image8 &outputImage);
void RGBA2Gray(const Image8 &inputImage, Image8 &outputImage);//IGNORES Alpha channel!!
void calculateSearchSpace(Image8 &inputImage, unsigned int &ssTop, unsigned int &ssBottom, unsigned int &ssLeft, unsigned int &ssRight);
void Sobel(const Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void Subtract(const Image8 &inputImage1, const Image8 &inputImage2, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight);
void thresholdImage(Image8 &input, unsigned char threshold = 128);
void skipsmGaussBlur5(const Image8 &input, Image8 &output);
void SubImage(const Image8 &inputImage, Image8 &subImage, unsigned int tl_x, unsigned int tl_y);
void DrawBox (Image8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char* colors);
#endif
