#include "featureCalculator.h"

//'output' should have its memory allocated
void Image8ToOpenCV(const Image8 &input, IplImage* output)
{
	//TODO This could potentially be made faster
	for (unsigned int yIter = 0; yIter < input.getHeight(); yIter++)
		for (unsigned int xIter = 0; xIter < input.getWidth(); xIter++)
			((uchar*)(output->imageData + output->widthStep*yIter))[xIter] = input.read(yIter,xIter);
}

/*This takes takes a list of chips that are all of the same target size and gray and output a model file
*/

void GradientMagnitudeDirection(const Image8 &inputImage, std::vector<double> &featureVector)
{
	assert(inputImage.getChannels() == 1);
    double sumx, sumy, mag;
    unsigned int width = inputImage.getWidth();
    unsigned int height = inputImage.getHeight();

    for (unsigned int i = 1; i < height - 1; i++)
            for (unsigned int j = 1; j < width - 1; j++)
            {
                    sumx = -inputImage.read(i-1,j-1) + inputImage.read(i-1,j+1) - 2 * inputImage.read(i,j-1) + 2 * inputImage.read(i,j+1) - inputImage.read(i+1,j-1) + inputImage.read(i+1,j+1);
                    sumy = inputImage.read(i-1,j-1) + 2 * inputImage.read(i-1,j) + inputImage.read(i-1,j+1) - inputImage.read(i+1,j-1) - 2 * inputImage.read(i+1,j) - inputImage.read(i+1,j+1);
                    mag = sqrt(sumx*sumx + sumy*sumy);
                    featureVector.push_back(sumx / 1020);
                    featureVector.push_back(sumy / 1020);
                    featureVector.push_back(mag / 1442.5);
            }
}

void heightWidthRatio(const Image8 &inputImage, std::vector<double> &featureVector)
{
        featureVector.push_back((double)inputImage.getHeight()/(double)inputImage.getWidth());
}

//From paper "Histograms of Oriented Gradients for Human Detection"
void histogramOrientedGradients(const Image8 &inputImage, std::vector<double> &featureVector)
{
	unsigned int channels = inputImage.getChannels();
	unsigned int cellSize = 8;//In pixels (x and y)
	unsigned int blockSize = 16;//In pixels (x and y)
	unsigned int numBins = 9;//Binning is 0-180 deg
	unsigned int numCellsX = (inputImage.getWidth() - 2) / cellSize;
	unsigned int numCellsY = (inputImage.getHeight() - 2) / cellSize;
	unsigned int numCells = numCellsX * numCellsY;
	unsigned int blockStride = 1;//In Cells
	unsigned int cellsPerBlockSide = blockSize / cellSize;
	static bool displayInfo = true;

	if (displayInfo)
	{
		cout << "NumCells: " << numCells << " Cell Size: " << cellSize << " Block Size: " << blockSize << " Block Stride: " << blockStride << " Num Bins: " << numBins << " NumCellsX: " << numCellsX << " NumCellsY: " << numCellsY << " CellsPerBlockSide: " << cellsPerBlockSide << endl;
		displayInfo = false;
	}
	if (blockSize % cellSize != 0 && (inputImage.getWidth() - 2) % blockSize != 0 || (inputImage.getHeight() - 2) % blockSize != 0)
	{
		cout << "ERROR: The HOG feature is implemented such that the resulting gradient mask (remember, you lose 2 rows and cols due to the convolution) must be evenly divisible by the cells and blocks!" << endl;
		cout << "Height: " << inputImage.getHeight() << " Width: " << inputImage.getWidth() << endl;
		exit(1);
	}

	//Calculate orientation and magnitude for image.  We will use [-1 0 1] as our derivative mask, and ignore the 1 pixel border
	unsigned int derivativeSize = (inputImage.getHeight())*(inputImage.getWidth());//NOTE, some space won't be used, this is for simplicity during addressing (1 pixel border)
	double *x_derivative = new double[channels*derivativeSize];
	double *y_derivative = new double[channels*derivativeSize];
	double *theta_map = new double[derivativeSize];
	double *norm_map = new double[derivativeSize];
	
	unsigned int widthOffset = inputImage.getWidth();

	//Dump all of the norm and angle info TODO Remove when this is confirmed to work
	static unsigned int chipCount = 0;
	char tempName[100];
	
	sprintf(tempName,"Theta%u.txt",chipCount);
	FILE* thetaFile = fopen(tempName,"w");
	
	sprintf(tempName,"Norm%u.txt",chipCount);
	FILE* normFile = fopen(tempName,"w");
	double *temp_norm = new double[channels];
	for (unsigned int yIter = 1; yIter < inputImage.getHeight() - 1; yIter++)
		for (unsigned int xIter = 1; xIter < inputImage.getWidth() - 1; xIter++)
		{
			unsigned int tempPos = yIter*widthOffset + xIter;
			//Calculate x/y derivative and norm
			for (unsigned int chanIter = 0; chanIter < channels; chanIter++)
			{
				x_derivative[channels*(tempPos) + chanIter] = -inputImage.read(yIter,xIter-1,chanIter) + inputImage.read(yIter,xIter+1,chanIter);
				y_derivative[channels*(tempPos) + chanIter] = -inputImage.read(yIter-1,xIter,chanIter) + inputImage.read(yIter+1,xIter,chanIter);
				temp_norm[chanIter] = sqrt(x_derivative[channels*(tempPos) + chanIter]*x_derivative[channels*(tempPos) + chanIter] + y_derivative[channels*(tempPos) + chanIter]*y_derivative[channels*(tempPos) + chanIter]);
			}
			
			//Find max norm, move norm and derivative to final map
			double max_norm = 0;
			unsigned int max_norm_index = 0;
			for (unsigned int chanIter = 0; chanIter < channels; chanIter++)
			{
				if (temp_norm[chanIter] > max_norm)
				{
					max_norm = temp_norm[chanIter];
					max_norm_index = chanIter;
				}
			}
			norm_map[tempPos] = max_norm;
			unsigned int normArrayIndex = channels*(tempPos) + max_norm_index;
			if (x_derivative[normArrayIndex] == 0)
				x_derivative[normArrayIndex] = .0000001;//TODO Change to smallest non-zero positive number
			theta_map[tempPos] = atan(y_derivative[normArrayIndex]/x_derivative[normArrayIndex]);
			
			fprintf(thetaFile,"%f ",theta_map[tempPos]);
			fprintf(normFile,"%f ",norm_map[tempPos]);
		}
	
	delete [] temp_norm;
	fclose(thetaFile);
	fclose(normFile);
	
	double **cellHistograms = new double*[numCells];
	for (unsigned int cellIter = 0; cellIter < numCells; cellIter++)
	{
		cellHistograms[cellIter] = new double[numBins];
		for (unsigned int binIter = 0; binIter < numBins; binIter++)
			cellHistograms[cellIter][binIter] = 0;
	}
	
	unsigned int cellCounter = 0;
	//Create 2d cell map, each holding a histogram of the orientations (double)
	for (unsigned int yIter = 1; yIter < inputImage.getHeight() - 1; yIter+=cellSize)
		for (unsigned int xIter = 1; xIter < inputImage.getWidth() - 1; xIter+=cellSize)
		{
			for (unsigned int yCellIter = 0; yCellIter < cellSize; yCellIter++)
				for (unsigned int xCellIter = 0; xCellIter < cellSize; xCellIter++)
				{
					unsigned int tempPos = (yIter+yCellIter)*widthOffset + xIter + xCellIter;
					unsigned int orientationBin = (unsigned int)floor((theta_map[tempPos] + 1.5708)/.3491);
					cellHistograms[cellCounter][orientationBin] += norm_map[tempPos];	
				}
				
				//NOTE This is a debugging output to show one orientation histogram
				/*unsigned int testBin = 32;
				if (cellCounter == testBin)
				{
					cout << "Orientation hist @ cell[" << testBin << "] - ";
					for (unsigned int binIter = 0; binIter < numBins; binIter++)
					{
						cout << cellHistograms[testBin][binIter] << " ";
					}
					cout << endl;
				}*/
				cellCounter++;
		}
	
	//For every 'block' position, find the norm, then normalize each cell's histogram with it, and it to the classification vector
	//For these we are calculating based on blocks
	for (unsigned int yCellIter = 0; yCellIter < numCellsY - blockStride; yCellIter+= blockStride)
		for (unsigned int xCellIter = 0; xCellIter < numCellsX - blockStride; xCellIter+= blockStride)
		{
			double tempSumOfSquares = 0;
			//Get sum of squares, calculate norm, go through again and normalize the histograms, add them to the feature vector
			for (unsigned int yBlockIter = 0; yBlockIter < cellsPerBlockSide; yBlockIter++)
				for (unsigned int xBlockIter = 0; xBlockIter < cellsPerBlockSide; xBlockIter++)
				{
					for (unsigned int binIter = 0; binIter < numBins; binIter++)
					{
						double tempVal = cellHistograms[(yCellIter + yBlockIter)*numCellsX + xCellIter + xBlockIter][binIter];
						tempSumOfSquares += tempVal * tempVal;
					}
				}
				
				double blockNorm = sqrt(tempSumOfSquares);
				for (unsigned int yBlockIter = 0; yBlockIter < cellsPerBlockSide; yBlockIter++)
					for (unsigned int xBlockIter = 0; xBlockIter < cellsPerBlockSide; xBlockIter++)
					{
							for (unsigned int binIter = 0; binIter < numBins; binIter++)
							{
								double tempVal = cellHistograms[(yCellIter + yBlockIter)*numCellsX + xCellIter + xBlockIter][binIter];
								featureVector.push_back(tempVal/blockNorm);
							}
					}
		}
	for (unsigned int cellIter = 0; cellIter < numCells; cellIter++)
	{
		delete [] cellHistograms[cellIter];
	}
	delete [] cellHistograms;
	
	delete [] x_derivative;
	delete [] y_derivative;
	delete [] theta_map;
	delete [] norm_map;
}

//From paper "Human Detection Using Oriented Histograms of Flow and Appearance"
void histogramOrientedFlow(const Image8 &inputImage1, const Image8 &inputImage2,std::vector<double> &featureVector)
{
	assert(inputImage1.getHeight() == inputImage2.getHeight());
	assert(inputImage1.getWidth() == inputImage2.getWidth());
	unsigned int height = inputImage1.getHeight();
	unsigned int width = inputImage1.getWidth();
	unsigned int channels = inputImage1.getChannels();
	unsigned int cellSize = 8;//In pixels (x and y)
	unsigned int blockSize = 16;//In pixels (x and y)
	unsigned int numBins = 9;//Binning is 0-180 deg
	unsigned int numCellsX = (width - 2) / cellSize;
	unsigned int numCellsY = (height - 2) / cellSize;
	unsigned int numCells = numCellsX * numCellsY;
	unsigned int blockStride = 1;//In Cells
	unsigned int cellsPerBlockSide = blockSize / cellSize;
	static bool displayInfo = true;

	if (displayInfo)
	{
		cout << "HOF - NumCells: " << numCells << " Cell Size: " << cellSize << " Block Size: " << blockSize << " Block Stride: " << blockStride << " Num Bins: " << numBins << " NumCellsX: " << numCellsX << " NumCellsY: " << numCellsY << " CellsPerBlockSide: " << cellsPerBlockSide << endl;
		displayInfo = false;
	}
	if (blockSize % cellSize != 0 || (width - 2) % blockSize != 0 || (height - 2) % blockSize != 0)
	{
		cout << "ERROR: The HOF feature is implemented such that the resulting gradient mask (remember, you lose 2 rows and cols due to the convolution) must be evenly divisible by the cells and blocks!" << endl;
		cout << "Height: " << height << " Width: " << width << endl;
		exit(1);
	}

	unsigned int derivativeSize = (height)*(width);//NOTE, some space won't be used, this is for simplicity during addressing (1 pixel border)
	double *x_flow = new double[derivativeSize];
	double *y_flow = new double[derivativeSize];
	
	unsigned int widthOffset = inputImage2.getWidth();

	const Image8 *grayImage1;
	const Image8 *grayImage2;

	bool image1Created = false;
	bool image2Created = false;

	if (inputImage1.getChannels() == 1)
		grayImage1 = &inputImage1;
	else
	{
		image1Created = true;
		grayImage1 = new Image8(height, width, 1);
		RGB2Gray(inputImage1,*((Image8*)grayImage1));
	}

	if (inputImage2.getChannels() == 1)
		grayImage2 = &inputImage2;
	else
	{
		image2Created = true;
		grayImage2 = new Image8(inputImage2.getHeight(), inputImage2.getWidth(), 1);
		RGB2Gray(inputImage2,*((Image8*)grayImage2));
	}

	//Convert input images to openCV images
	CvSize imSize = cvSize(width, height);
	IplImage* cvImage1 = cvCreateImage(imSize,IPL_DEPTH_8U,channels);
	IplImage* cvImage2 = cvCreateImage(imSize,IPL_DEPTH_8U,channels);
	Image8ToOpenCV(*grayImage1,cvImage1);
	Image8ToOpenCV(*grayImage2,cvImage2);
	
	//Optical Flow
	const double lambda = .1;//It appears that when this gets smaller, the magnitude of the vectors gets larger
	CvTermCriteria criteria;
	criteria.type = CV_TERMCRIT_ITER;
	criteria.max_iter = 64;
	criteria.epsilon = .01;
	
	CvMat* velx= cvCreateMat(height,width,CV_32FC1);
	CvMat* vely= cvCreateMat(height,width,CV_32FC1);

	cvSetZero(velx);
	cvSetZero(vely);

	cvCalcOpticalFlowHS(cvImage1, cvImage2,0,velx,vely,lambda,criteria);
	if (0)
	{
		static unsigned int chip_counter = 0;
		//TODO Remove, this just saves the flow info
		char file_name[100];
		sprintf(file_name,"output/%u-flowx.txt",chip_counter);
		FILE *flow_file_x = fopen(file_name,"w");
		sprintf(file_name,"output/%u-flowxpos.txt",chip_counter);
		FILE *flow_file_x_pos = fopen(file_name,"w");
		sprintf(file_name,"output/%u-flowy.txt",chip_counter);
		FILE *flow_file_y = fopen(file_name,"w");
		sprintf(file_name,"output/%u-flowypos.txt",chip_counter);
		FILE *flow_file_y_pos = fopen(file_name,"w");

		for (unsigned int yIter = 0; yIter < height; yIter++)
		{
			for (unsigned int xIter = 0; xIter < width; xIter++)
			{
				fprintf(flow_file_x,"%f ",CV_MAT_ELEM( *velx, float, yIter, xIter ));
				fprintf(flow_file_x_pos,"%u ",xIter);
				fprintf(flow_file_y,"%f ",CV_MAT_ELEM( *vely, float, yIter, xIter ));
				fprintf(flow_file_y_pos,"%u ",yIter);
			}
			fprintf(flow_file_x,"\n");
			fprintf(flow_file_y,"\n");
			fprintf(flow_file_x_pos,"\n");
			fprintf(flow_file_y_pos,"\n");
		}
		fclose(flow_file_x);
		fclose(flow_file_y);
		fclose(flow_file_x_pos);
		fclose(flow_file_y_pos);
		chip_counter++;
	}
	//Copy CV arrays to the doubles
	for (unsigned int yIter = 0; yIter < height; yIter++)
	{
		for (unsigned int xIter = 0; xIter < width; xIter++)
		{
			x_flow[yIter*width + xIter] = CV_MAT_ELEM( *velx, float, yIter, xIter );
			y_flow[yIter*width + xIter] = CV_MAT_ELEM( *vely, float, yIter, xIter );
		}
	}
	//Free all opencv data
	cvReleaseImage(&cvImage1);
	cvImage1 = 0;
	cvReleaseImage(&cvImage2);
	cvImage2 = 0;
	cvReleaseMat(&velx);
	velx = 0;
	cvReleaseMat(&vely);
	vely = 0;

	double **cellHistograms = new double*[numCells];
	for (unsigned int cellIter = 0; cellIter < numCells; cellIter++)
	{
		cellHistograms[cellIter] = new double[numBins];
		for (unsigned int binIter = 0; binIter < numBins; binIter++)
			cellHistograms[cellIter][binIter] = 0;
	}
	
	unsigned int cellCounter = 0;
	/* Spatial difference calculation and binning
	1.  Each cell becomes the "center" cell
	2.  All neighboring cells pixels are subtracted by the center cell's pixels (across both flow fields, the angle and magnitude is calculated, then the binned (into cell's histogram)
	*/
	unsigned int inputBorderSize = 1;
	unsigned int upperHeight = height - inputBorderSize;//Values used must be LESS THAN these upper limits
	unsigned int upperWidth = width - inputBorderSize;
	for (unsigned int yIter = inputBorderSize; yIter < upperHeight; yIter+=cellSize)
		for (unsigned int xIter = inputBorderSize; xIter < upperWidth; xIter+=cellSize)
		{
			assert(cellCounter < numCells);
			//Selects relative neighbor pixel distances
			for (unsigned int yRelCellIter = 0; yRelCellIter < cellsPerBlockSide; yRelCellIter++)
				for (unsigned int xRelCellIter = 0; xRelCellIter < cellsPerBlockSide; xRelCellIter++)
					//Iterates over each pixel in the cell
					for (unsigned int yCellIter = 0; yCellIter < cellSize; yCellIter++)
						for (unsigned int xCellIter = 0; xCellIter < cellSize; xCellIter++)
						{
							int yRelativePos = (yRelCellIter - cellsPerBlockSide/2)*cellSize;
							int xRelativePos = (xRelCellIter - cellsPerBlockSide/2)*cellSize;
							if (yRelativePos == 0 && xRelativePos == 0)//We skip the center cell
								continue;

							int yNeighborPos = yIter + yCellIter + yRelativePos;
							int xNeighborPos = xIter + xCellIter + xRelativePos;
							//NOTE These comparisons are sign safe
							if (yNeighborPos < inputBorderSize || yNeighborPos >= upperHeight || xNeighborPos < inputBorderSize || xNeighborPos >= upperWidth)
								continue;

							unsigned int tempCenterPos = (yIter+yCellIter)*widthOffset + xIter + xCellIter;
							unsigned int tempNeighborPos = (yNeighborPos)*widthOffset + xNeighborPos;
							double differenceX = x_flow[tempNeighborPos] - x_flow[tempCenterPos];
							double differenceY = y_flow[tempNeighborPos] - y_flow[tempCenterPos];
							if (differenceX == 0.0)
								differenceX = .000000001;//Arbitratily small constant TODO Change to smallest non-zero number
							unsigned int orientationBin = (unsigned int)floor((atan(differenceY/differenceX) + 1.5708)/.3491);
							cellHistograms[cellCounter][orientationBin] += sqrt(differenceX*differenceX + differenceY*differenceY);
						}
			cellCounter++;
		}
	
	//For every 'block' position, find the norm, then normalize each cell's histogram with it, add it to the classification vector
	//For these we are calculating based on blocks
	for (unsigned int yCellIter = 0; yCellIter < numCellsY - blockStride; yCellIter+= blockStride)
		for (unsigned int xCellIter = 0; xCellIter < numCellsX - blockStride; xCellIter+= blockStride)
		{
			double tempSumOfSquares = 0;
			//Get sum of squares, calculate norm, go through again and normalize the histograms, add them to the feature vector
			for (unsigned int yBlockIter = 0; yBlockIter < cellsPerBlockSide; yBlockIter++)
				for (unsigned int xBlockIter = 0; xBlockIter < cellsPerBlockSide; xBlockIter++)
				{
					for (unsigned int binIter = 0; binIter < numBins; binIter++)
					{
						double tempVal = cellHistograms[(yCellIter + yBlockIter)*numCellsX + xCellIter + xBlockIter][binIter];
						tempSumOfSquares += tempVal * tempVal;
					}
				}
				
				double blockNorm = sqrt(tempSumOfSquares);
				for (unsigned int yBlockIter = 0; yBlockIter < cellsPerBlockSide; yBlockIter++)
					for (unsigned int xBlockIter = 0; xBlockIter < cellsPerBlockSide; xBlockIter++)
					{
						for (unsigned int binIter = 0; binIter < numBins; binIter++)
						{
							double tempVal = cellHistograms[(yCellIter + yBlockIter)*numCellsX + xCellIter + xBlockIter][binIter];
							featureVector.push_back(tempVal/blockNorm);
						}
					}
		}
	for (unsigned int cellIter = 0; cellIter < numCells; cellIter++)
	{
		delete [] cellHistograms[cellIter];
	}
	delete [] cellHistograms;
	
	delete [] x_flow;
	delete [] y_flow;
	if (image1Created)
		delete grayImage1;
	if (image2Created)
		delete grayImage2;
}


void featureCalculation(vector<double> &currentVector, unsigned int featureMode, Image8 *currentChip, Image8* currentChipTwin)
{
	switch(featureMode)
	{
		case 0://Do simple sobel Partial X, Partial Y, Magnitude
			GradientMagnitudeDirection(*currentChip, currentVector);
			break;
		case 1://Use HoG
			histogramOrientedGradients(*currentChip, currentVector);
			break;
		case 2://Use Sobel and HoG
			GradientMagnitudeDirection(*currentChip, currentVector);
			histogramOrientedGradients(*currentChip, currentVector);
			break;
		case 3://Just use flow
			assert(currentChipTwin);
			histogramOrientedFlow(*currentChip,*currentChipTwin, currentVector);
			break;
		case 4://Flow and HOG
			assert(currentChipTwin);
			histogramOrientedFlow(*currentChip,*currentChipTwin, currentVector);
			histogramOrientedGradients(*currentChip, currentVector);
			break;
		case 5://Flow, HOG, and Sobel
			assert(currentChipTwin);
			GradientMagnitudeDirection(*currentChip, currentVector);
			histogramOrientedFlow(*currentChip,*currentChipTwin, currentVector);
			histogramOrientedGradients(*currentChip, currentVector);
			break;
		default:
			cerr << "Invalid 'feature mode'" << endl;
			exit(1);
	}
}
