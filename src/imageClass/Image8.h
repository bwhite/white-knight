/**
 * A very simple image class with a 1 byte underlying representation.  As this is the fundemental unit in image processing, I feel its important for it to be as general purpose and easy to use as possible.
 *
 * \author Brandyn White
 */
//TODO Remove assertions and create something functionally the same but allow for it to be turned off with a different define than -DNDEBUG as these need to be turned off for speed, and hte others we might want for quality
#ifndef IMAGE8_H
#define IMAGE8_H
#include <assert.h>
#include <string.h>
class Image8
{
private:
	unsigned char *data; //Points to our allocated memory
	const unsigned int height;
	const unsigned int width;
	const unsigned int channel;
	const unsigned int size;
public:
	Image8(unsigned int height, unsigned int width, unsigned int channel = 1, bool zero_image = 0);
	~Image8();
	void zero()
	{
		memset(data, 0, size);
	}

	unsigned int getWidth() const
	{
		return width;
	}

	unsigned int getHeight() const
	{
		return height;
	}

	unsigned int getChannels() const
	{
		return channel;
	}

	unsigned int getByteSize() const
	{
		return size;
	}

	inline unsigned char read(unsigned int height, unsigned int width, unsigned int channel = 0) const//aka (row, column,channel) aka (y, x,channel)
	{
		#ifdef DEBUG
		assert(height < this->height);
		assert(width < this->width);
		assert(channel < this->channel);
		#endif
		return data[this->channel * (this->width * height + width) + channel];
	}

	inline void write(unsigned int height, unsigned int width, unsigned int channel, unsigned char value)//aka (row, column,channel, value) aka (y, x,channel, value)
	{
		#ifdef DEBUG
		assert(height < this->height);
		assert(width < this->width);
		assert(channel < this->channel);
		#endif
		data[this->channel * (this->width * height + width) + channel] = value;
	}

	//NOTE This shouldn't generally be used by anything but high performance code segments such as image reading!
	inline unsigned char* getDataPointer()
	{
		return data;
	}
	
	//Used for memory reading (note the const)
	inline const unsigned char* getSafeDataPointer() const
	{
		return data;
	}
	
	//Used to get the direct access to the channels of a specific pixel, user is responsible for channel bounds
	inline const unsigned char* getPixelPointer(unsigned int height, unsigned int width) const//aka (row, column) aka (y, x)
	{
		#ifdef DEBUG
		assert(height < this->height);
		assert(width < this->width);
		#endif
		return &(data[this->channel * (this->width * height + width)]);
	}

	inline void copyFrom(const Image8& image)
	{
		assert(image.getHeight() == this->height);
		assert(image.getWidth() == this->width);
		assert(image.getChannels() == this->channel);
		memcpy(this->data, image.getSafeDataPointer(), size);
	}
	//TODO Create simple array transformations to make it easier for to convert different image formats to this one
	//These can be used to convert basic image representations into this image format, next to each is how you access the target pixel value (the capitalized values are the predefined maxima for this image)
	//inline void copyFrom(unsigned char *data)//For row major 1-D packed data:  CHANNEL * (WIDTH * height + width) + channel
	//inline void copyFrom(unsigned char ***data)//For row major 3-D packed data:  [height][width][channel]
};
#endif
