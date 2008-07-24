#include "imageTools.h"
#include <cmath>
#include <cstdlib>
//This checks for nonzero pixels and finds the best fit box for that, this can be used to optimize any function that modifies a whole image through the use of a kernel
//If nothing is found, top will be the height, bottom will be 0, left will be width, and right will be 0, thus all "for" loops using this shouldn't even go one pass
void calculateSearchSpace(Image8 &inputImage, unsigned int &ssTop, unsigned int &ssBottom, unsigned int &ssLeft, unsigned int &ssRight)
{
	assert(inputImage.getChannels() == 1);
	ssTop = inputImage.getHeight();
	ssBottom = 0;
	ssLeft = inputImage.getWidth();
	ssRight = 0;
	bool anySet = false;

	if (ssBottom > inputImage.getHeight() - 1)
		ssBottom = inputImage.getHeight() - 1;
	if (ssRight > inputImage.getWidth() - 1)
		ssRight = inputImage.getWidth() - 1;

	for (unsigned int i = 0; i< inputImage.getHeight(); i++)
	{
		for (unsigned int j = 0; j < inputImage.getWidth(); j++)
		{
			if (inputImage.read(i,j))
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

void BilinearResample(Image8 &input, Image8 &output)
{
    double dX = (input.getWidth() - 1.0) / output.getWidth();
    double dY = (input.getHeight() - 1.0) / output.getHeight();
 
    for (unsigned int i = 0; i < output.getHeight(); i++)
    {
    	for (unsigned int j = 0; j < output.getWidth(); j++)
        {
			for (unsigned int k = 0; k < output.getChannels(); k++)
			{
	        	double distX = j * dX;
	            double distY = i * dY;
 
	            unsigned int X = (unsigned int) floor(distX);
	            unsigned int Y = (unsigned int) floor(distY);
 
	            distX = distX - X;
	            distY = distY - Y;
 
	            double c1 = input.read(Y,X, k);
	            double c2 = input.read(Y,X+1, k);
	            double c3 = input.read(Y+1,X, k);
	            double c4 = input.read(Y+1,X+1, k);
 
	            double d1 = c1*(1-distX) + c2*distX;
	            double d2 = c3*(1-distX) + c4*distX;
	            double e1 = d1*(1-distY) + d2*distY;
 
	            output.write(i,j,k,(unsigned char) round(e1));
			}
		}
	}
}

void RGB2Gray(const Image8 &inputImage, Image8 &outputImage)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 3);
	assert(outputImage.getChannels() == 1);

	for (unsigned int i = 0; i < inputImage.getHeight();i++)
		for (unsigned int j = 0; j < inputImage.getWidth();j++)
		{
			outputImage.write(i,j,0,(unsigned char)round(.3*inputImage.read(i,j,0) + .59*inputImage.read(i,j,1) + .11*inputImage.read(i,j,2)));
		}
}

//NOTE This will simply ignore the alpha channel
void RGBA2Gray(const Image8 &inputImage, Image8 &outputImage)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 4);
	assert(outputImage.getChannels() == 1);

	for (unsigned int i = 0; i < inputImage.getHeight();i++)
		for (unsigned int j = 0; j < inputImage.getWidth();j++)
		{
			outputImage.write(i,j,0,(unsigned char)round(.3*inputImage.read(i,j,0) + .59*inputImage.read(i,j,1) + .11*inputImage.read(i,j,2)));
		}
}

void Gray2RGB(const Image8 &inputImage, Image8 &outputImage)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 1);
	assert(outputImage.getChannels() == 3);

	for (unsigned int i = 0; i < inputImage.getHeight();i++)
		for (unsigned int j = 0; j < inputImage.getWidth();j++)
		{
			for (unsigned int k = 0; k < 3; k++)
				outputImage.write(i,j,k,inputImage.read(i,j));
		}
}

//TODO Convert this to work like scanlines, doing memcpy for each row of the output!
void SubImage (const Image8 &inputImage, Image8 &subImage, unsigned int tl_x, unsigned int tl_y)
{
	assert(tl_y + subImage.getHeight() <= inputImage.getHeight());
	assert(tl_x + subImage.getWidth() <= inputImage.getWidth());
	for (unsigned int i = tl_y; i < tl_y + subImage.getHeight();i++)
		for (unsigned int j = tl_x; j < tl_x + subImage.getWidth();j++)
			for (unsigned int k = 0; k < subImage.getChannels(); k++)
				subImage.write(i-tl_y, j-tl_x,k,inputImage.read(i,j,k));
}

void DrawBox (Image8 &inputImage, unsigned int tl_x, unsigned int tl_y, unsigned int br_x, unsigned int br_y, unsigned char* colors)
{
	for (unsigned int k = tl_x; k <= br_x; k++)
	{
		for (unsigned int chanIter = 0; chanIter < inputImage.getChannels(); chanIter++)
			inputImage.write(tl_y, k,chanIter,colors[chanIter]);

		for (unsigned int chanIter = 0; chanIter < inputImage.getChannels(); chanIter++)
			inputImage.write(br_y, k,chanIter,colors[chanIter]);
	}


	for (unsigned int k = tl_y; k <= br_y; k++)
	{
		for (unsigned int chanIter = 0; chanIter < inputImage.getChannels(); chanIter++)
			inputImage.write(k, tl_x,chanIter,colors[chanIter]);

		for (unsigned int chanIter = 0; chanIter < inputImage.getChannels(); chanIter++)
			inputImage.write(k, br_x,chanIter,colors[chanIter]);
	}
}

void Sobel(const Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(inputImage.getChannels() == 1);
	assert(outputImage.getChannels() == 1);
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
			outputImage.write(i,j,0,total_sum/8);
		}
}

void Subtract(const Image8 &inputImage1, const Image8 &inputImage2, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage1.getHeight());
	assert(outputImage.getWidth() == inputImage1.getWidth());
	assert(outputImage.getHeight() == inputImage2.getHeight());
	assert(outputImage.getWidth() == inputImage2.getWidth());
	assert(inputImage1.getChannels() == 1);
	assert(inputImage2.getChannels() == 1);
	assert(outputImage.getChannels() == 1);

	for (unsigned int i = ssTop; i < ssBottom; i++)
		for (unsigned int j = ssLeft; j < ssRight; j++)
		{
			int result = inputImage1.read(i,j) - inputImage2.read(i,j);
			if (result > 0)
			{
				outputImage.write(i,j,0,(unsigned int) result);
			}
			else
				outputImage.write(i,j,0,0);
		}
}

//Takes in a gray image, and thresholds it (if >= threshold, then set it to 255, else 0)
void thresholdImage(Image8 &input, unsigned char threshold)
{
	assert(input.getChannels() == 1);
	for (unsigned int yIter = 0; yIter < input.getHeight(); yIter++)
		for (unsigned int xIter = 0; xIter < input.getWidth(); xIter++)
		{
			if (input.read(yIter,xIter) >= threshold)
				input.write(yIter,xIter,0,255);
			else
				input.write(yIter,xIter,0,0);
		}
}

//Skips a neighborhood of 2 on all sides!
void skipsmGaussBlur5(const Image8 &input, Image8 &output)
{
	assert(input.getChannels() == 1);
	assert(output.getChannels() == 1);
	unsigned int height = input.getHeight();
	unsigned int width = input.getWidth();

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
			Tmp1 = input.read(j,i);
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
			output.write(j-2,i-2,0,(128 + SC3[i] + Tmp2)/256);
			SC3[i] = Tmp2;
		}

	delete [] SC0;
	delete [] SC1;
	delete [] SC2;
	delete [] SC3;
}
