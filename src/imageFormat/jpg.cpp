#include <cstdlib>
#include <cctype>
#include <cassert>
#include "jpg.h"
#include <cstdio>

extern "C"
{
#include <jpeglib.h>
}

#define MAX_LINE 100
JPEG::JPEG()
{
	handledExtensions.push_back("jpeg");
	handledExtensions.push_back("jpg");
	handledChannelSizes.push_back(1);
	handledChannelSizes.push_back(3);
}

JPEG::~JPEG()
{}

bool JPEG::getImageHeader(unsigned int &height, unsigned int &width, unsigned int &channels, const char * fName)
{
	FILE* infile = fopen(fName, "rb");
	if(!infile)
	{
		errorCantOpenFile(fName);
		return false;
	}

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);

	jpeg_read_header(&cinfo, TRUE);

	height = cinfo.image_height;
	width = cinfo.image_width;
	channels = cinfo.num_components;
	jpeg_destroy_decompress(&cinfo);

	fclose(infile);
	return true;
}
bool JPEG::readImage(Image8 &image, const char * fName)
{
	if (image.getChannel() == 3)
	{
		FILE* infile = fopen(fName, "rb");
		if(!infile)
		{
			errorCantOpenFile(fName);
			return false;
		}

		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		cinfo.out_color_space = JCS_RGB;
		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, infile);
		jpeg_read_header(&cinfo, TRUE);

		unsigned int width = cinfo.image_width;
		unsigned int height = cinfo.image_height;
		assert(image.getWidth() == width);
		assert(image.getHeight() == height);

		jpeg_start_decompress(&cinfo);
		unsigned char *data = image.getDataPointer();
		//Create pointer array
		unsigned char ** pntArray = new unsigned char*[height];
		for (unsigned int i = 0; i < height; i++)
		{
			pntArray[i] = (data + width*3*i);
		}
		
		for (unsigned int i = 0; i < height; i++)
		{
			jpeg_read_scanlines(&cinfo, pntArray + i, 1);
		}
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		
		delete [] pntArray;

		fclose(infile);
		return true;
	}
	else if (image.getChannel() == 1)
	{
		FILE* infile = fopen(fName, "rb");
		if(!infile)
		{
			errorCantOpenFile(fName);
			return false;
		}

		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.out_color_space = JCS_RGB;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, infile);
		jpeg_read_header(&cinfo, TRUE);

		unsigned int width = cinfo.image_width;
		unsigned int height = cinfo.image_height;
		assert(image.getWidth() == width && image.getHeight() == height);

		jpeg_start_decompress(&cinfo);
		if (cinfo.output_components == 1)
		{
			unsigned char *buffer = new unsigned char[width];
			for (unsigned int i = 0; i < height; i++)
			{
				jpeg_read_scanlines(&cinfo, &buffer, 1);
				for (unsigned int j = 0; j < width; j++)
					image(i,j) = buffer[j];
			}
			delete[] buffer;
		}
		else
		{
			unsigned char *buffer = new unsigned char[width*3];
			for (unsigned int i = 0; i < height; i++)
			{
				jpeg_read_scanlines(&cinfo, &buffer, 1);
				for (unsigned int j = 0; j < width; j++)
					image(i,j) = (unsigned char) (int) (buffer[j*3+0]*0.3 + buffer[j*3+1]*0.59 + buffer[j*3+2]*0.11);
			}
			delete[] buffer;
		}
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(infile);
		return true;
	}
	return false;
}
bool JPEG::writeImage(Image8 &image, const char * fName)
{
	if (image.getChannel() == 3)
	{
		FILE* outfile = fopen(fName, "wb");
		if(!outfile)
		{
			errorCantOpenFile(fName);
			return false;
		}

		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		jpeg_stdio_dest(&cinfo, outfile);

		unsigned int width = image.getWidth();
		unsigned int height = image.getHeight();

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		jpeg_set_defaults(&cinfo);

		jpeg_start_compress(&cinfo, TRUE);
		unsigned char *buffer = new unsigned char[width*3];
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
				for (unsigned int k = 0; k < 3; k++)
					buffer[3*j + k] = image(i,j,k);
			jpeg_write_scanlines(&cinfo, &buffer, 1);
		}
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		delete[] buffer;

		fclose(outfile);
		return true;
	}
	else if (image.getChannel() == 1)
	{
		FILE* outfile = fopen(fName, "wb");

		if(!outfile)
		{
			printf("Couldn't open file %s\n", fName);
			return false;
		}

		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		jpeg_stdio_dest(&cinfo, outfile);

		unsigned int width = image.getWidth();
		unsigned int height = image.getHeight();

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = 1;
		cinfo.in_color_space = JCS_GRAYSCALE;

		jpeg_set_defaults(&cinfo);

		jpeg_start_compress(&cinfo, TRUE);
		unsigned char *buffer = new unsigned char[width];
		for (unsigned int i = 0; i < height; i++)
		{
			for (unsigned int j = 0; j < width; j++)
				buffer[j] = image(i,j);
			jpeg_write_scanlines(&cinfo, &buffer, 1);
		}
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		delete[] buffer;

		fclose(outfile);
		return true;
	}
	return false;
}
