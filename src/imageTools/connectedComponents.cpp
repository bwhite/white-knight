#include "connectedComponents.h"
#include <iostream>
using namespace std;
//This function takes a gray image, thresholds at (BG) 127/128 (FG), and returns a list of connected objects

//Recursive 4-connected scan - Takes a list<unsigned int[2]>, adds to it all of the points that belong to (including) the seed.  Unsets the pixel when found
void ccRecursive4(Image8 &image, unsigned int xPos, unsigned int yPos, list<connectedPoint_t> &objectPoints)
{
	if (image.read(yPos,xPos) >= 128)
	{
		image.write(yPos,xPos,0,0);
		connectedPoint_t tempPoint;
		tempPoint.x = xPos;
		tempPoint.y = yPos;
		objectPoints.push_back(tempPoint);
		
		if (xPos > 0)
			ccRecursive4(image,xPos - 1,yPos,objectPoints);
		if (yPos > 0)
			ccRecursive4(image,xPos,yPos - 1,objectPoints);
		if (xPos + 1 < image.getWidth())
			ccRecursive4(image,xPos + 1,yPos,objectPoints);
		if (yPos + 1 < image.getHeight())
			ccRecursive4(image,xPos,yPos + 1,objectPoints);
	}
}
/*class CoordStack
{
public:
	void push(unsigned int x, unsigned int y)
	{
		xStack.push_back(x);
		yStack.push_back(y);
	}
	bool pop(unsigned int &x, unsigned int &y)
	{
		if (xStack.empty() || yStack.empty())
			return false;
		
		x = xStack.front();
		y = yStack.front();
		
		xStack.pop_front();
		yStack.pop_front();
		return true;
	}
protected:
	list<unsigned int> xStack;
	list<unsigned int> yStack;
};
*/
class CoordStack
{
public:
	CoordStack()
	{
		xStack.reserve(100);//arbitrarily large values
		yStack.reserve(100);
	}
	void push(unsigned int x, unsigned int y)
	{
		xStack.push_back(x);
		yStack.push_back(y);
	}
	bool pop(unsigned int &x, unsigned int &y)
	{
		if (xStack.empty() || yStack.empty())
			return false;
		
		x = xStack.back();
		y = yStack.back();
		
		xStack.pop_back();
		yStack.pop_back();
		return true;
	}
	void empty()
	{
		xStack.clear();
		yStack.clear();
	}
protected:
	vector<unsigned int> xStack;
	vector<unsigned int> yStack;
};


CoordStack stack;//Note this global variable is here, it makes this process much faster
//TODO Convert all of these into a class so that we can get rid of the global variable

//The scanline floodfill algorithm using our own stack routines, faster
void floodFillScanlineStack(Image8 &image, unsigned int xPos, unsigned int yPos, list<connectedPoint_t> &objectPoints)
{
	stack.empty();
    unsigned int yIter; 
    bool spanLeft, spanRight;
	const unsigned int width = image.getWidth();
	const unsigned int height = image.getHeight();
    
    stack.push(xPos, yPos);
    
    while(stack.pop(xPos, yIter))
    {
		//Here the edge case is quite difficult because we are using unsigned int's so we can't allow it to wrap around
        while(image.read(yIter,xPos) >= 128 && yIter > 0)
			yIter--;
			
		if (image.read(yIter,xPos) < 128)
			yIter++;

        spanLeft = spanRight = false;

        while(yIter < height && image.read(yIter,xPos) >= 128)
        {
			//Unset this point, and save it to the object's point list
			image.write(yIter,xPos,0,0);
			connectedPoint_t tempPoint;
			tempPoint.x = xPos;
			tempPoint.y = yIter;
			objectPoints.push_back(tempPoint);
			
            if(!spanLeft && xPos > 0 && image.read(yIter,xPos - 1) >= 128) 
            {
                stack.push(xPos - 1, yIter);
                spanLeft = true;
            }
            else if(spanLeft && xPos > 0 && image.read(yIter, xPos - 1) < 128)
                spanLeft = false;

            if(!spanRight && xPos < width - 1 && image.read(yIter, xPos + 1) >= 128) 
            {
                stack.push(xPos + 1, yIter);
                spanRight = true;
            }
            else if(spanRight && xPos < width - 1 && image.read(yIter,xPos + 1) < 128)
                spanRight = false;

            yIter++;                   
        }
    }        
}

void getConnectedObjects(Image8 &image, list<list<connectedPoint_t> > &connectedObjects, bool scanline)
{
	assert(image.getChannels() == 1);//Check if image is grayscale
	
	Image8 workingImage(image.getHeight(), image.getWidth(), image.getChannels());
	//Make a copy of the image so that we can unset the pixels as we like
	workingImage.copyFrom(image);
	
	if (scanline)
	{
		//Go through every pixel, running the recursive connected algorithm, for each pixel, create a new list which will be passed to the recursive algorithm
		for (unsigned int yIter = 0; yIter < image.getHeight(); yIter++)
			for(unsigned int xIter = 0; xIter < image.getWidth(); xIter++)
			{
				if (image.read(yIter,xIter) >= 128)
				{
					list<connectedPoint_t> tempPoints;
					floodFillScanlineStack(workingImage, xIter, yIter, tempPoints);
					if (tempPoints.size() <= 0)
						continue;
					connectedObjects.push_back(tempPoints);
				}
			}
	}
	else
	{
		//Go through every pixel, running the recursive connected algorithm, for each pixel, create a new list which will be passed to the recursive algorithm
		for (unsigned int yIter = 0; yIter < image.getHeight(); yIter++)
			for(unsigned int xIter = 0; xIter < image.getWidth(); xIter++)
			{
				if (image.read(yIter,xIter) >= 128)
				{
					list<connectedPoint_t> tempPoints;
					ccRecursive4(workingImage, xIter, yIter, tempPoints);
					if (tempPoints.size() <= 0)
						continue;
					connectedObjects.push_back(tempPoints);
				}
		}
	}
}
