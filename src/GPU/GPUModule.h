namespace GPUModule
{
	int HEIGHT;
	int WIDTH;
	// CAUTION! The height and width are UNSIGNED
	void Init(unsigned int height, unsigned int width);
	
	/* You have to pack and unpack all of the data for the GPU like this:
	
	   inone: [GaussianR GaussianG GaussianB GaussianVariance]
	   intwo: [ImageR ImageG ImageB]
	   out:   [NewGaussianR NewGaussianG NewGaussianB NewGaussianVariance]
	
	   Unfold this structure for each pixel in the standard way, one line 
	   at a time.
	*/
	void UpdateGaussian(unsigned int height, float alpha, const float *inone, const float *intwo, float *out);
	
	// Please call this with a separate thread
	void EventLoop(); 
	
	void Destroy();
}
