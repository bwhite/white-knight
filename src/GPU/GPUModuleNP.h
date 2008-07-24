namespace GPUModuleNP
{
	void Init(unsigned int window, unsigned int height, unsigned int width);
	
	/* 
	   You have to pack and unpack all of the data for the GPU like this:
	
	   Unfold this structure for each pixel in the standard way, one line 
	   at a time.
	*/
	
	void FillBuffer(const float *in);
	
	void Subtract(const float *in, float *out, float v1, float v2, float v3,
				bool update = false);
	
	// Please call this with a separate thread
	void EventLoop(); 
	
	void Destroy();
}
