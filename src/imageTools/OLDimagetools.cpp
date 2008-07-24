#include "imagetools.h"
#include "pgm.h"
#include <cmath>
#include <iostream>

using namespace std;

//Input: currentImage, backgroundImage, and differenceImage	Output: deshadowedImage
//Searches for "on" pixels in the difference image and decides if they should be turned "off" due to their high possibility of being a shadow
void RemoveShadow(const ColorImage8 &currentImage, const ColorImage8 &backgroundImage, const GreyImage8 &differenceImage, GreyImage8 &deshadowedImage,std::list<configElement_t> configList, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(currentImage.getHeight() == backgroundImage.getHeight());
	assert(currentImage.getWidth() == backgroundImage.getWidth());
	assert(backgroundImage.getHeight() == differenceImage.getHeight());
	assert(backgroundImage.getWidth() == differenceImage.getWidth());
	assert(deshadowedImage.getHeight() == differenceImage.getHeight());
	assert(deshadowedImage.getWidth() == differenceImage.getWidth());

	double Vra = .1;
	double Vrb = .75;
	double Tsa = -.05;//-.06;
	double Tsb = .1;
	double Th = .15;

	Vra = FindValue(configList, "Vra", Vra);
	Vrb = FindValue(configList, "Vrb", Vrb);
	Tsa = FindValue(configList, "Tsa", Tsa);
	Tsb = FindValue(configList, "Tsb", Tsb);
	Th = FindValue(configList, "Th", Th);
	deshadowedImage = differenceImage;

	for (unsigned int i = ssTop; i < ssBottom; i++)
		for (unsigned int j = ssLeft; j < ssRight; j++)
		{
			if (differenceImage.read(i,j))
			{
				double cH, cS, cV, bH, bS, bV;
				RGB2HSV(currentImage.read(i,j,0),currentImage.read(i,j,1),currentImage.read(i,j,2),cH, cS, cV);
				RGB2HSV(backgroundImage.read(i,j,0),backgroundImage.read(i,j,1),backgroundImage.read(i,j,2),bH, bS, bV);
				double valueRatio = cV/bV;
				double saturationDiff = bS - cS;
				double absHueDiff = abs(bH - cH);
				//cout << "(" << i << "," << j << ") VR: " << valueRatio << " SD: " << saturationDiff << " HD: " << absHueDiff << endl;
				if ((Vra <= valueRatio) && (valueRatio <= Vrb) && (Tsa <= saturationDiff) && (saturationDiff <= Tsb) && (absHueDiff <= Th))
				{
					//cout << "Shadow detected!" << endl;
					deshadowedImage(i,j) = 0;
				}
			}

		}
}

/*
Given an larger image, a <= sized image, and a coordinate for the top left corner, we will copy the contents from the first image to the second image, starting a the given coordinate, and going through the height/width of the second image 
*/
void SubImage (const ColorImage8 &inputImage, ColorImage8 &subImage, unsigned int tl_x, unsigned int tl_y)
{
	assert(tl_y + subImage.getHeight() <= inputImage.getHeight());
	assert(tl_x + subImage.getWidth() <= inputImage.getWidth());
	for (unsigned int i = tl_y; i < tl_y + subImage.getHeight();i++)
		for (unsigned int j = tl_x; j < tl_x + subImage.getWidth();j++)
			for (unsigned int k = 0; k < 3; k++)
				subImage(i-tl_y, j-tl_x,k) = inputImage.read(i,j,k);
}

void SubImage (const GreyImage8 &inputImage, GreyImage8 &subImage, unsigned int tl_x, unsigned int tl_y)
{
	assert(tl_y + subImage.getHeight() <= inputImage.getHeight());
	assert(tl_x + subImage.getWidth() <= inputImage.getWidth());
	for (unsigned int i = tl_y; i < tl_y + subImage.getHeight();i++)
		for (unsigned int j = tl_x; j < tl_x + subImage.getWidth();j++)
			subImage(i-tl_y, j-tl_x) = inputImage.read(i,j);
}

void Subtract(const GreyImage8 &inputImage1, const GreyImage8 &inputImage2, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage1.getHeight());
	assert(outputImage.getWidth() == inputImage1.getWidth());
	assert(outputImage.getHeight() == inputImage2.getHeight());
	assert(outputImage.getWidth() == inputImage2.getWidth());

	for (unsigned int i = ssTop; i < ssBottom; i++)
		for (unsigned int j = ssLeft; j < ssRight; j++)
		{
			int result = inputImage1.read(i,j) - inputImage2.read(i,j);
			if (result > 0)
			{
				outputImage(i,j) = (unsigned int) result;
			}
			else
				outputImage(i,j) = 0;
		}
}

void RGB2HSV(unsigned char r, unsigned char g, unsigned char b, double &h, double &s, double &v)
{
	double tempRed = r / 255.0;                     //RGB values = 0 ÷ 255
	double tempGreen = g / 255.0;
	double tempBlue = b / 255.0;

	double cmin, cmax, delta;
	cmax = (tempRed > tempGreen) ? tempRed : tempGreen;
	cmax = (cmax > tempBlue) ? cmax : tempBlue;
	cmin = (tempRed < tempGreen) ? tempRed : tempGreen;
	cmin = (cmin < tempBlue) ? cmin : tempBlue;
	delta = cmax - cmin;
	v = cmax;
	s = (cmax != 0.0) ? delta / cmax : 0.0;
	if (s == 0.0)
		h = 0.0;  // No hue.
	else
	{
		if (tempRed == cmax)
			h = (((tempBlue > tempGreen) ? 6.0 : 0.0) + (tempGreen - tempBlue) / delta) / 6.0;
		else if (tempGreen == cmax)
			h = (2.0 + (tempBlue - tempRed) / delta) / 6.0;
		else
			h = (4.0 + (tempRed - tempGreen) / delta) / 6.0;
	}
}

void Subtract(const GreyImage8 &inputImage1, const GreyImage8 &inputImage2, GreyImage8 &outputImage)
{
	Subtract(inputImage1,inputImage2, outputImage, 0, inputImage1.getHeight(), 0, inputImage2.getWidth());
}

void RGB2Grey(const ColorImage8 &inputImage, GreyImage8 &outputImage)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());

	for (unsigned int i = 0; i < inputImage.getHeight();i++)
		for (unsigned int j = 0; j < inputImage.getWidth();j++)
		{
			outputImage(i,j) = (unsigned char)round(.3*inputImage.read(i,j,0) + .59*inputImage.read(i,j,1) + .11*inputImage.read(i,j,2));
		}
}

void Grey2RGB(const GreyImage8 &inputImage, ColorImage8 &outputImage)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());

	for (unsigned int i = 0; i < inputImage.getHeight();i++)
		for (unsigned int j = 0; j < inputImage.getWidth();j++)
		{
			outputImage(i,j,0) = outputImage(i,j,1) = outputImage(i,j,2) = inputImage.read(i,j);
		}
}

//This is a "dumb" dilate that exploits the fast that we generally have more background than foreground, a switch can be added later to decide which to use depending on this ratio
void Dilate(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());

	if (ssTop < 1)
		ssTop = 1;
	if (ssLeft < 1)
		ssLeft = 1;
	if (ssBottom > inputImage.getHeight() - 2)
		ssBottom = inputImage.getHeight() - 2;
	if (ssRight > inputImage.getWidth() - 2)
		ssRight = inputImage.getWidth() - 2;

	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			if (inputImage(i,j))
			{
				for (unsigned int k = i - (ySize-1)/2;k < i + (ySize-1)/2; k++)
					for (unsigned int l = j - (xSize-1)/2;l < j + (xSize-1)/2; l++)
					{
						outputImage(k,l) = 255;
					}
			}
		}
}

//This doesn't do checking to make sure that we can fit a kernel on the outer edge, it should be checked for BEFORE inputting the parameters here
void Erode(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());

	if (ssTop < 1)
		ssTop = 1;
	if (ssLeft < 1)
		ssLeft = 1;
	if (ssBottom > inputImage.getHeight() - 2)
		ssBottom = inputImage.getHeight() - 2;
	if (ssRight > inputImage.getWidth() - 2)
		ssRight = inputImage.getWidth() - 2;

	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			if (inputImage(i,j))
			{
				outputImage(i,j) = 255;
				bool continueLoop = true;
				for (unsigned int k = i - (ySize-1)/2;k < i + (ySize-1)/2 && continueLoop; k++)
					for (unsigned int l = j - (xSize-1)/2;l < j + (xSize-1)/2 && continueLoop; l++)
					{
						if (!inputImage(k,l))
						{
							outputImage(i,j) = 0;
							continueLoop = false;
						}
					}
			}
		}
}


void Erode(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage)
{
	Erode(xSize, ySize, inputImage, outputImage, (ySize-1)/2, inputImage.getHeight() - (ySize-1)/2 - 1, (xSize-1)/2, inputImage.getWidth() - (xSize-1)/2 - 1);
}

void Dilate(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage)
{
	Dilate(xSize, ySize, inputImage, outputImage, (ySize-1)/2, inputImage.getHeight() - (ySize-1)/2 - 1, (xSize-1)/2, inputImage.getWidth() - (xSize-1)/2 - 1);
}

//This checks for nonzero pixels and finds the best fit box for that, this can be used to optimize any function that modifies a whole image through the use of a kernel
//If nothing is found, top will be the height, bottom will be 0, left will be width, and right will be 0, thus all "for" loops using this shouldn't even go one pass
void calculateSearchSpace(GreyImage8 &inputImage, unsigned int &ssTop, unsigned int &ssBottom, unsigned int &ssLeft, unsigned int &ssRight)
{
	ssTop = inputImage.getHeight();
	ssBottom = 0;
	ssLeft = inputImage.getWidth();
	ssRight = 0;
	bool anySet = false;

	if (ssBottom > inputImage.getHeight() - 1)
		ssBottom = inputImage.getHeight() - 1;
	if (ssRight > inputImage.getWidth() - 1)
		ssRight = inputImage.getWidth() - 1;

	for (unsigned int i = 0; i< inputImage.getHeight();i++)
	{
		for (unsigned int j = 0; j < inputImage.getWidth(); j++)
		{
			if (inputImage(i,j))
			{
				anySet = true;
				if (i < ssTop)
					ssTop = i;
				else if (i > ssBottom)
					ssBottom = i;
				if (j < ssLeft)
					ssLeft = j;
				else if (j > ssRight)
					ssRight = j;
			}
		}
	}

	/* Set a default search window if none are found, preferably not on an edge so that later algorithms aren't getting messed up
	 * For example, if we do for (unsigned int i = ssTop; i <= ssBottom - 10; i++)
	 * 				for (unsigned int j = ssLeft; j <= ssRight - 10; j++)
	 * We will overflow the variable and go into a very long loop out of bounds of the memory which will crash the system
	 */
	if (!anySet)
	{
		ssTop = ssBottom = inputImage.getHeight() / 2;
		ssLeft = ssRight = inputImage.getWidth() / 2;
		ssTop++;
		ssLeft++;
	}
}

//Searches -(x or y)Size to join pixels as the top will join to the bottom as with the left joining with the right
void joinComponents(unsigned int xSize, unsigned int ySize, GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());


	if (ssBottom + ySize > inputImage.getHeight())
		ssBottom = inputImage.getHeight() - ySize;

	if (ssRight + xSize > inputImage.getWidth())
		ssRight = inputImage.getWidth() - xSize;

	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			if (inputImage(i,j))
			{
				/* Explination: This algorithm looks for on pixels, it then searches in the increasing direction for both x and y, if it immediately finds a "set" pixel, then we know that
				 * In that direction we are not on an edge and that part will be explored later on.  If we find a "set" pixel after a gap, we use that position and set all the pixels leading up to that
				*/

				//Search Y
				bool foundY = false;
				bool firstY = true;
				unsigned int k; //The value from the iteration is needed after this loop, so this var is external to it

				for (k = i + 1;k < i + ySize;k++)
				{
					if (inputImage(k,j))
					{
						if (firstY)
							break;
						else
							foundY = true;
					}
					firstY = false;
				}
				if (foundY)
					for (unsigned int l = i;l < k;l++)
						outputImage(l,j) = 255;

				//Search X
				bool foundX = false;
				bool firstX = true;

				for (k = j + 1;k < j + xSize;k++)
				{
					if (inputImage(i,k))
					{
						if (firstX)
							break;
						else
							foundX = true;
					}
					firstX = false;
				}
				if (foundX)
					for (unsigned int l = j;l < k;l++)
						outputImage(i,l) = 255;
			}
		}
}

void fillBox(GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char color)
{
	for (unsigned int i = tl_y; i <= br_y; i++)
		for (unsigned int j = tl_x; j <= br_x; j++)
			inputImage(i,j) = color;
}


//Color is an index into the array in the function, it does modulus the value so we can't exceed it, its useful for bounding box colors with respect to object label numbers
void drawBox (GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned int color)
{
	unsigned char colors[] = {0, 15, 31, 47, 63, 79, 95, 111, 127, 143, 159, 175, 191, 207, 223, 239, 255};

	unsigned char currentColor = colors[color%sizeof(colors)];
	for (unsigned int k = tl_x; k <= br_x; k++)
	{
		inputImage(tl_y, k) = currentColor;
		inputImage(br_y, k) = currentColor;
	}

	for (unsigned int k = tl_y; k <= br_y; k++)
	{
		inputImage(k, tl_x) = currentColor;
		inputImage(k, br_x) = currentColor;
	}
}

//Color is an index into the array in the function, it does modulus the value so we can't exceed it, its useful for bounding box colors with respect to object label numbers
void drawBox (GreyImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y)
{
	for (unsigned int k = tl_x; k <= br_x; k++)
	{
		inputImage(tl_y, k) = 128;
		inputImage(br_y, k) = 128;
	}

	for (unsigned int k = tl_y; k <= br_y; k++)
	{
		inputImage(k, tl_x) = 128;
		inputImage(k, br_x) = 128;
	}
}

void drawBox (ColorImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y)
{
	for (unsigned int k = tl_x; k <= br_x; k++)
	{
		inputImage(tl_y, k,0) = 0;
		inputImage(tl_y, k,1) = 255;
		inputImage(tl_y, k,2) = 0;

		inputImage(br_y, k,0) = 0;
		inputImage(br_y, k,1) = 255;
		inputImage(br_y, k,2) = 0;
	}


	for (unsigned int k = tl_y; k <= br_y; k++)
	{
		inputImage(k, tl_x,0) = 0;
		inputImage(k, tl_x,1) = 255;
		inputImage(k, tl_x,2) = 0;

		inputImage(k, br_x,0) = 0;
		inputImage(k, br_x,1) = 255;
		inputImage(k, br_x,2) = 0;
	}
}
void drawBox (ColorImage8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char red, unsigned char green, unsigned char blue)
{
	for (unsigned int k = tl_x; k <= br_x; k++)
	{
		inputImage(tl_y, k,0) = red;
		inputImage(tl_y, k,1) = green;
		inputImage(tl_y, k,2) = blue;

		inputImage(br_y, k,0) = red;
		inputImage(br_y, k,1) = green;
		inputImage(br_y, k,2) = blue;
	}


	for (unsigned int k = tl_y; k <= br_y; k++)
	{
		inputImage(k, tl_x,0) = red;
		inputImage(k, tl_x,1) = green;
		inputImage(k, tl_x,2) = blue;

		inputImage(k, br_x,0) = red;
		inputImage(k, br_x,1) = green;
		inputImage(k, br_x,2) = blue;
	}
}


/* Connected Components
 * We are given an image and a search region, and we will look for set pixels, which we will accumulate into objects which we will also compute the centroid, best fit box, etc
 */
//This exploits the fact that objects can't be touching the border of the accessable image, if this behavior changes, this will also have to change
void ccRecursive(GreyImage8 &differenceImage, const GreyImage8 &edgeDifference, const GreyImage8 &inputImage, connectedObject_t &object, unsigned int i, unsigned int j, unsigned int &sumx, unsigned int &sumy, unsigned int &meanSum)
{
	if (differenceImage(i,j))
	{
		differenceImage(i,j) = 0;
		object.area++;
		sumy += i;
		sumx += j;
		if (edgeDifference.read(i,j) > 3)
			object.edge_size++;

		meanSum += inputImage.read(i,j);

		if (j < object.tl_x)
			object.tl_x = j;
		if (i < object.tl_y)
			object.tl_y = i;
		if (j > object.br_x)
			object.br_x = j;
		if (i > object.br_y)
			object.br_y = i;

		if (i > 0)
			ccRecursive(differenceImage, edgeDifference, inputImage, object, i - 1, j, sumx, sumy, meanSum);
		if (j > 0)
			ccRecursive(differenceImage, edgeDifference, inputImage, object, i, j - 1, sumx, sumy, meanSum);
		if (i + 1 < inputImage.getHeight())
			ccRecursive(differenceImage, edgeDifference, inputImage, object, i + 1, j, sumx, sumy, meanSum);
		if (j + 1 < inputImage.getWidth())
			ccRecursive(differenceImage, edgeDifference, inputImage, object, i, j + 1, sumx, sumy, meanSum);
	}
}

//Used to gather blobs and populate objects
void connectedComponenets(GreyImage8 &differenceImage, GreyImage8 &tempImage, const GreyImage8 &edgeDifference, const GreyImage8 &inputImage, std::list<connectedObject_t> &objectList, unsigned int minArea,unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(tempImage.getHeight() == differenceImage.getHeight());
	assert(tempImage.getWidth() == differenceImage.getWidth());
	assert(edgeDifference.getHeight() == differenceImage.getHeight());
	assert(edgeDifference.getWidth() == differenceImage.getWidth());

	if (ssTop < 1)
		ssTop = 1;
	if (ssLeft < 1)
		ssLeft = 1;
	if (ssBottom > differenceImage.getHeight() - 2)
		ssBottom = differenceImage.getHeight() - 2;
	if (ssRight > differenceImage.getWidth() - 2)
		ssRight = differenceImage.getWidth() - 2;

	tempImage = differenceImage;
	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			if (tempImage(i,j))
			{
				connectedObject_t object;
				unsigned int sumx = 0;
				unsigned int sumy = 0;
				unsigned int meanSum = 0;
				object.area = 0;
				object.edge_size = 0;
				sumx = 0;
				sumy = 0;
				object.matchedThisFrame = false;
				object.br_x = 0;//These are set to values that force them to change
				object.br_y = 0;
				object.tl_x = ssRight + 1;
				object.tl_y = ssBottom + 1;
				ccRecursive(tempImage, edgeDifference,inputImage, object, i,j, sumx, sumy, meanSum);
				if (object.area > minArea)
				{
					object.seed_x = j;
					object.seed_y = i;
					object.center_x = sumx / (double)object.area;
					object.center_y = sumy / (double)object.area;
					object.meanColor = meanSum / (double)object.area;
					objectList.push_back(object);
				}
			}
		}
}

void Sobel(const GreyImage8 &inputImage, GreyImage8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	unsigned int total_sum;
	int sumx, sumy;

	if (ssTop < 1)
		ssTop = 1;
	if (ssLeft < 1)
		ssLeft = 1;
	if (ssBottom > inputImage.getHeight() - 2)
		ssBottom = inputImage.getHeight() - 2;
	if (ssRight > inputImage.getWidth() - 2)
		ssRight = inputImage.getWidth() - 2;

	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			total_sum = sumx = sumy = 0;

			sumx = -inputImage.read(i-1,j-1) + inputImage.read(i-1,j+1) - 2 * inputImage.read(i,j-1) + 2 * inputImage.read(i,j+1) - inputImage.read(i+1,j-1) + inputImage.read(i+1,j+1);
			sumy = inputImage.read(i-1,j-1) + 2 * inputImage.read(i-1,j) + inputImage.read(i-1,j+1) - inputImage.read(i+1,j-1) - 2 * inputImage.read(i+1,j) - inputImage.read(i+1,j+1);
			//This is the gradient magnitude approx, if this doesn't work well then we should use the real magnitude
			total_sum = abs(sumx) + abs(sumy);
			outputImage(i,j) = total_sum/8;
		}
}

void ThresholdGrey(GreyImage8 &inputImage, unsigned char threshold)
{
	for (unsigned int i = 0; i < inputImage.getHeight(); i++)
		for (unsigned int j = 0; j < inputImage.getWidth(); j++)
		{
			if (inputImage(i,j) >= threshold)
				inputImage(i,j) = 255;
			else
				inputImage(i,j) = 0;
		}
}

void getGaussianKernel(double *&kernel, double sigma, unsigned int radius)
{
	/* Use a one-dimensional vector as the kernel, since gaussian smoothing is separable */
	for (int i = -radius; i <= (int) radius; i++)
		kernel[i+radius] = 1/(sqrt(2*M_PI)*sigma)*exp(-i*i/(2*sigma*sigma));
}

//This is here if anybody needs it, but try to use the SKIPSM based blurs for speed
void GaussianBlur(const GreyImage8 &inputImage, GreyImage8 &outputImage, double *&kernel, unsigned int radius, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());

	unsigned int width = inputImage.getWidth();
	unsigned int height = inputImage.getHeight();

	if (ssTop < 1)
		ssTop = radius;
	if (ssLeft < 1)
		ssLeft = radius;
	if (ssBottom > inputImage.getHeight() - radius)
		ssBottom = inputImage.getHeight() - radius;
	if (ssRight > inputImage.getWidth() - radius)
		ssRight = inputImage.getWidth() - radius;

	unsigned int n = 2*radius + 1;

	double *buffer = new double[n];

	/* First go in the horizontal direction. Since only one additional input point is
	   needed at each iteration, and one is no longer needed, I use a circular buffer
	   so I need to read each point only once. */
	for (unsigned int i = ssTop; i < ssBottom; i++)
	{
		for (unsigned int j = 0; j < n - 1; j++)
			buffer[j] = inputImage.read(i,j);

		for (unsigned int j = ssLeft; j < ssRight; j++)
		{
			buffer[(j+radius)%n] = inputImage.read(i,j+radius);
			double sum = 0;
			for (unsigned int x = 0; x < n; x++)
				sum += kernel[x] * buffer[(x+j-radius)%n];
			outputImage(i,j) = (unsigned char) sum;
		}
	}

	/* Now do the vertical direction on the previous result. Since I'm storing
	   the points in a buffer as I need them, I can perform this operation in place,
	   overwriting data as I go, rather than allocating an intermediate image. */
	for (unsigned int j = ssLeft; j < ssRight; j++)
	{
		for (unsigned int i = 0; i < n - 1; i++)
			buffer[i] = outputImage.read(i,j);

		for (unsigned int i = ssTop; i < ssBottom; i++)
		{
			buffer[(i+radius)%n] = outputImage.read(i+radius,j);
			double sum = 0;
			for (unsigned int x = 0; x < n; x++)
				sum += kernel[x] * buffer[(x+i-radius)%n];
			outputImage(i,j) = (unsigned char) sum;
		}
	}

	/* Because I used outputImage as an intermediate buffer as well as the final image,
	   I have regions of unneeded data at the top and bottom of the image. This clears
	   them out. */
	for (unsigned int i = 0; i < radius; i++)
		for (unsigned int j = radius; j < width - radius; j++)
			outputImage(i,j) = 0;

	for (unsigned int i = height - radius; i < height; i++)
		for (unsigned int j = radius; j < width - radius; j++)
			outputImage(i,j) = 0;
	delete [] buffer;
}

//The blurs below are based off of the SKIPSM method
void GaussianBlur3(GreyImage8 &inputImage, GreyImage8 &outputImage)
{
	unsigned int height = inputImage.getHeight();
	unsigned int width = inputImage.getWidth();

	unsigned int Tmp1, Tmp2;
	unsigned int SR0;
	unsigned int SR1;
	unsigned int *SC0 = new unsigned int[width];
	unsigned int *SC1 = new unsigned int[width];

	for (unsigned int i = 0; i < width; i++)
		SC0[i] = SC1[i] = 0;

	for (unsigned int j = 1; j < height; j++)
		for (unsigned int i = (SR0=SR1=0)+1; i < width; i++)
		{
			Tmp1 = inputImage.read(j,i);
			Tmp2 = SR0 + Tmp1;
			SR0 = Tmp1;
			Tmp1 = SR1 + Tmp2;
			SR1 = Tmp2;

			Tmp2 = SC0[i] + Tmp1;
			SC0[i] = Tmp1;
			outputImage(j-1,i-1) = (8 + SC1[i] + Tmp2)/16;
			SC1[i] = Tmp2;
		}
	delete [] SC0;
	delete [] SC1;
}

void GaussianBlur5(GreyImage8 &inputImage, GreyImage8 &outputImage)
{
	unsigned int height = inputImage.getHeight();
	unsigned int width = inputImage.getWidth();

	unsigned int Tmp1, Tmp2;
	unsigned int SR0;
	unsigned int SR1;
	unsigned int SR2;
	unsigned int SR3;
	unsigned int *SC0 = new unsigned int[width];
	unsigned int *SC1 = new unsigned int[width];
	unsigned int *SC2 = new unsigned int[width];
	unsigned int *SC3 = new unsigned int[width];

	for (unsigned int i = 0; i < width; i++)
		SC0[i] = SC1[i] = SC2[i] = SC3[i] = 0;

	for (unsigned int j = 2; j < height; j++)
		for (unsigned int i = (SR0=SR1=SR2=SR3=0)+2; i < width; i++)
		{
			Tmp1 = inputImage.read(j,i);
			Tmp2 = SR0 + Tmp1;
			SR0 = Tmp1;
			Tmp1 = SR1 + Tmp2;
			SR1 = Tmp2;
			Tmp2 = SR2 + Tmp1;
			SR2 = Tmp1;
			Tmp1 = SR3 + Tmp2;
			SR3 = Tmp2;

			Tmp2 = SC0[i] + Tmp1;
			SC0[i] = Tmp1;
			Tmp1 = SC1[i] + Tmp2;
			SC1[i] = Tmp2;
			Tmp2 = SC2[i] + Tmp1;
			SC2[i] = Tmp1;
			outputImage(j-2,i-2) = (128 + SC3[i] + Tmp2)/256;
			SC3[i] = Tmp2;
		}
	delete [] SC0;
	delete [] SC1;
	delete [] SC2;
	delete [] SC3;
}
