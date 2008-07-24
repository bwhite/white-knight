/**
 * This class allows for a list of images to be provided to the caller
 *
 * \author Brandyn White
 */
#include <cstdio>
#include <iostream>
#include "ListLoader.h"

ListLoader::ListLoader(list<vector<string> > configList, list<ImageFormat*> formats, char* inputListName) : stateSaved(false)
{
	if (!inputListName)
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "FileList", fileListName);
		assert(!notLoaded);
	}
	else
	{
		strcpy(fileListName,inputListName);
	}
	
	//Load config settings
	int notLoaded = 0;
	notLoaded += FindValue(configList, "InputDir", inputDir);
	notLoaded += FindValue(configList, "InitialFrameNum", initialFrameNumber);
	notLoaded += FindValue(configList, "LineSkip", lineSkip);
	assert(!notLoaded);

	char tempPath[100];
	sprintf(tempPath, "%s/%s", inputDir, fileListName);

	cout << __FILE__ << " - Opening file list [" << tempPath << "]" << endl;
	fileList = fopen(tempPath,"r");
	assert(fileList);

	this->formats = formats;

	reset();

	//NOTE This gets the height,width,channel from the first file in the list, this will also go through the entire list to gather the total number of files listed
	char tempFileName[100];
	fscanf(fileList, " %s ", tempFileName);
	sprintf(tempPath, "%s/%s", inputDir, tempFileName);
	bool imageRead = (getFileFormat(tempFileName))->getImageHeader(height,width,channels,tempPath);
	if (!imageRead)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - Image not read correctly!" << endl;
		exit(1);
	}
	
	updateNumListedImages();
}

//This will go through the list of files and count how many there are, later support will be added to ensure the files actually exist, and also can be loaded
//NOTE This will reset the list!
void ListLoader::updateNumListedImages()
{
	reset();
	numListedImages = 0;
	bool firstImage = true;
	char tempFileName[100];
	do
	{
		fscanf(fileList, " %s ", tempFileName);
		if (firstImage)
		{
			firstImage = false;
			cout << "Filename [" << tempFileName << "] [" << numListedImages << "]\n";
			cout << "...\n";
		}
		numListedImages++;
	} while(!feof(fileList));
	cout << "Filename [" << tempFileName << "] [" << numListedImages << "]\n";
	reset();
}

ListLoader::~ListLoader()
{	
	for (list<ImageFormat*>::iterator formatIter = formats.begin(); formatIter != formats.end(); formatIter++)
		delete *formatIter;
	fclose(fileList);
}

void ListLoader::getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels)
{
	height = this->height;
	width = this->width;
	channels = this->channels;
}

void ListLoader::reset()
{
	frameCounter = initialFrameNumber;
	rewind(fileList);
	//Skip number of lines specified in config, this is necessary if you have some constant sized header
	for (unsigned int lineSkipIter = 0; lineSkipIter < lineSkip; lineSkipIter++)
		fscanf(fileList, " %*s ");
	returnedImages = 0;
}

///This returns a pointer to the file handler that accepts the given filename
ImageFormat* ListLoader::getFileFormat(const char *fileName)
{
	//Find the last period, and say everything after that is the extension, if not found then the string will be null
	char * periodLocation = strrchr(fileName, '.');
	if (periodLocation == NULL)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - Your image files must have a period delimited extension!" << endl;
		cerr << "Filename input was [" << fileName << "]" << endl;
		exit(1);
	}

	//Copy everything after period
	char tempExtension[100];
	strcpy(tempExtension, ++periodLocation);

	//Find a format that will handle it
	bool matchFound = false;
	list<ImageFormat*>::iterator formatIter;
	for (formatIter = formats.begin(); formatIter != formats.end(); formatIter++)
		if ((*formatIter)->isHandled(tempExtension))
		{
			matchFound = true;
			break;
		}

	if (!matchFound)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - Extension [" << tempExtension << "] not found!" << endl;
		exit(1);
	}
	//Move that format to the front of the formats list in an effort to reduce search time
	if (formatIter != formats.begin())
		formats.splice(formats.begin(),formats, formatIter);
	return *formatIter;
}

void ListLoader::getImage (Image8 *image) throw (NoMoreImages)
{
	//Increment the file pointer, frameCounter, and returnedImages
	if (!image)
	{
			//Get file name from list
			char tempFileName[100];
			if (fscanf(fileList, " %s ", tempFileName) <= 0)
			{
				cerr << __FILE__ << ":" << __LINE__ << " - No more images in file list (not necessarily an error)." << endl;
				throw NoMoreImages("No more files available in the list!");
			}

			char tempPath[100];
			sprintf(tempPath, "%s/%s", inputDir, tempFileName);

		#ifndef NDEBUG
			cout << "Frame: " << frameCounter << " File: " << tempPath << endl;
		#endif

			returnedImages++;
			frameCounter++;
	}
	else
	{
		getImage(*image);
	}
}

void ListLoader::getImage (Image8 &image) throw (NoMoreImages)
{
	assert(image.getHeight() == height);
	assert(image.getWidth() == width);
	assert(image.getChannels() == channels);

	//Get file name from list
	char tempFileName[100];
	if (fscanf(fileList, " %s ", tempFileName) <= 0)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - No more images in file list (not necessarily an error)." << endl;
		throw NoMoreImages("No more files available in the list!");
	}

	char tempPath[100];
	sprintf(tempPath, "%s/%s", inputDir, tempFileName);

#ifndef NDEBUG
	cout << "Frame: " << frameCounter << " File: " << tempPath << endl;
#endif
	
	returnedImages++;
	frameCounter++;

	//Call format with image
	bool imageRead = (getFileFormat(tempFileName))->readImage(image,tempPath);
	if (!imageRead)
	{
		cerr << __FILE__ << ":" << __LINE__ << " - Image not read correctly!" << endl;
		exit(1);
	}
}
