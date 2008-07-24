#ifndef _NONPARAMETRICGPU_H_
#define _NONPARAMETRICGPU_H_
#include "../configTools/configReader.h"
#include "../ImageProvider.h"
#include "../BackgroundSubtraction.h"

using namespace std;

class NonParametricGPU : public BackgroundSubtraction
{
public:
	NonParametricGPU(unsigned int height, unsigned int width, unsigned int channels, ImageProvider *imProvide, list<vector<string> > configList);/* Height, Width, config, image handler */
	~NonParametricGPU();
	void subtract (Image8 &differenceImage, Image8 &imageRan);
	virtual void hint(const char * name, Image8 &hintImage)
	{
		cout << "HINT: " << name << "not supported!\n";
	}
	virtual void hint(const char * name, double hintValue)
	{
		cout << "HINT: " << name << "not supported!\n";
	}
	virtual void resetHints()
	{
	}
private:
	unsigned int height;
	unsigned int width;
	unsigned int channels;
	
	unsigned int windowSize;
	
	unsigned int frameSkip;
	double sphereVariance;
	double backgroundThreshold;
	
	//GPU
	
	float *currentGPUImage;//For GPU transfer
	float *outputImage;//From GPU
	ImageProvider *imProvide;
};
#endif /* _NONPARAMETRICGPU_H_ */
