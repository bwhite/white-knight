#include "binaryImageTools.h"

//Searches -(x or y)Size to join pixels as the top will join to the bottom as with the left joining with the right
//Output image should be a copy of the input image
void joinComponents(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 1);
	assert(outputImage.getChannels() == 1);

	if (ssBottom + ySize > inputImage.getHeight())
		ssBottom = inputImage.getHeight() - ySize;

	if (ssRight + xSize > inputImage.getWidth())
		ssRight = inputImage.getWidth() - xSize;

	for (unsigned int i = ssTop; i <= ssBottom; i++)
		for (unsigned int j = ssLeft; j <= ssRight; j++)
		{
			if (inputImage.read(i,j))
			{
				/* Explanation: This algorithm looks for on pixels, it then searches in the increasing direction for both x and y, if it immediately finds a "set" pixel, then we know that
				 * In that direction we are not on an edge and that part will be explored later on.  If we find a "set" pixel after a gap, we use that position and set all the pixels leading up to that
				*/

				//Search Y
				bool foundY = false;
				bool firstY = true;
				unsigned int k; //The value from the iteration is needed after this loop, so this var is external to it

				for (k = i + 1;k < i + ySize;k++)
				{
					if (inputImage.read(k,j))
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
						outputImage.write(l,j,0,255);

				//Search X
				bool foundX = false;
				bool firstX = true;

				for (k = j + 1;k < j + xSize;k++)
				{
					if (inputImage.read(i,k))
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
						outputImage.write(i,l,0,255);
			}
		}
}

//This is a "dumb" dilate that exploits the fast that we generally have more background than foreground, a switch can be added later to decide which to use depending on this ratio
void Dilate(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 1);
	assert(outputImage.getChannels() == 1);

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
			if (inputImage.read(i,j))
			{
				for (unsigned int k = i - (ySize-1)/2;k < i + (ySize-1)/2; k++)
					for (unsigned int l = j - (xSize-1)/2;l < j + (xSize-1)/2; l++)
					{
						outputImage.write(k,l,0,255);
					}
			}
		}
}

//This doesn't do checking to make sure that we can fit a kernel on the outer edge, it should be checked for BEFORE inputting the parameters here
void Erode(unsigned int xSize, unsigned int ySize, Image8 &inputImage, Image8 &outputImage, unsigned int ssTop, unsigned int ssBottom, unsigned int ssLeft, unsigned int ssRight)
{
	assert(outputImage.getHeight() == inputImage.getHeight());
	assert(outputImage.getWidth() == inputImage.getWidth());
	assert(inputImage.getChannels() == 1);
	assert(outputImage.getChannels() == 1);

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
			if (inputImage.read(i,j))
			{
				outputImage.write(i,j,0,255);
				bool continueLoop = true;
				for (unsigned int k = i - (ySize-1)/2;k < i + (ySize-1)/2 && continueLoop; k++)
					for (unsigned int l = j - (xSize-1)/2;l < j + (xSize-1)/2 && continueLoop; l++)
					{
						if (!inputImage.read(k,l))
						{
							outputImage.write(i,j,0,0);
							continueLoop = false;
						}
					}
			}
		}
}
