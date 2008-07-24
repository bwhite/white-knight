#include "../imageClass/Image8.h"
#include "ChangeDetector.h"
#include <cmath>
#include <iostream>

// TODO Fix the edge cases of filling up the buffer in small sequences

ChangeDetector::ChangeDetector(list<vector<string> > configList, ImageProvider *imProvider)
	: imProvider(imProvider) 
{
	assert(imProvider);
	
	unsigned int notLoaded = 0;
	notLoaded += FindValue(configList, "ChangeDetectionWindow", window);
	notLoaded += FindValue(configList, "ChangeDetectionWidth", width);
	notLoaded += FindValue(configList, "ChangeDetectionHeight", height);
	limitedRange = !FindValue(configList, "ChangeDetectionEnd", end);
	assert(!notLoaded);
}

void ChangeDetector::detectChanges(vector<double> &changeScore)
{
	unsigned int channels;
	unsigned int imWidth;
	unsigned int imHeight;

	Image8 **buffer = new Image8*[window];
	
	imProvider->getImageInfo(imHeight, imWidth, channels);
	
	Image8 originalImage(imHeight, imWidth, channels);
	
	for (unsigned int i = 0; i < window; i++)
		buffer[i] = new Image8(height, width, 1);
		
	for (unsigned int i = 0; i < window / 2; i++)
		changeScore.push_back(0.0);
	
	for (unsigned int i = 0; i < end || !limitedRange; i++)
	{
		try
		{
			imProvider->getImage(originalImage);
		}
		catch (NoMoreImages)
		{
			break;
		}
		
		// The current image is always added to the last spot in the ring buffer, i%window

		for (unsigned int m = 0; m < height; m++)
		{
			for (unsigned int n = 0; n < width; n++)
			{
				unsigned int y = m * imHeight / height;
				unsigned int x = n * imWidth / width; 
				
				if (channels == 3)
					buffer[i%window]->write(m,n,0,(unsigned char)round(.3*originalImage.read(y,x,0) + .59*originalImage.read(y,x,1) + .11*originalImage.read(y,x,2)));
				else
					buffer[i%window]->write(m,n,0,originalImage.read(y,x));
			}
		}
		
		if (i < window - 1)
			continue;
			
		double score = 0;

		for (unsigned int m = 0; m < height; m++)
		{
			for (unsigned int n = 0; n < width; n++)
			{
				double sum_sq_x = 0;
				double sum_sq_y = 0;
				double sum_coproduct = 0;
				double mean_x = 0;
				double mean_y = buffer[(i+1)%window]->read(m,n);
				
				// The index of the first element is (i+1)%window
		
				
				for (unsigned int j = 1; j < window; j++)
				{
					double sweep = j /(double)(j+1); 
					double delta_x = j - mean_x;
					double delta_y = buffer[(j+i+1)%window]->read(m,n) - mean_y;
		
					sum_sq_x += delta_x * delta_x * sweep;
					sum_sq_y += delta_y * delta_y * sweep;
		
					sum_coproduct += delta_x * delta_y * sweep;
					mean_x += delta_x / (j+1);
					mean_y += delta_y / (j+1);
				}
				
				double pop_sd_x = sqrt(sum_sq_x / window);
				double pop_sd_y = sqrt(sum_sq_y / window);
					
				double cov_x_y = sum_coproduct / window;
		
				double correlation = cov_x_y / (pop_sd_x * pop_sd_y);
		
				if (isnan(correlation))
					correlation = 0;
					
				//score += (pop_sd_y>5) * correlation;//TODO Andrew says this may be useful
				score += correlation*correlation;
				//cout << "Mess:  " << pop_sd_y << " " << correlation << " " << sum_sq_x << " " << sum_sq_y << mean_y << mean_x << sum_sq_x << sum_sq_y << endl;
			}
		}
		changeScore.push_back(score / (width * height));
	}
	for (unsigned int i = 0; i < window; i++)
		delete buffer[i];
	delete buffer;
	
	imProvider->reset();
}
