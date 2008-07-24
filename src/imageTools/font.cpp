#include <cstdio>
#include <cstring>
#include "font.h"
#include "imageTools.h"
#include "../imageFormat/devil.h"

const char *FONT_STRING = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]{}().,/+-* ";
#define WIDTH 8
#define HEIGHT 12

Font::Font()
{
	for (int i = 0; i < 256; i++)
		table[i] = 0;

	n_chars = strlen(FONT_STRING);

	for (int i = 1; i < n_chars; i++)
		table[(unsigned int) FONT_STRING[i]] = i;

	font = new Image8(12, WIDTH * n_chars,1);
	Devil devil;
	int returnVal = devil.readImage(*font, "font.pgm");
	if (!returnVal)
	{
		cout << "FONT:  Font image not found!" << endl;
		exit(1);
	}
	chars = new Image8* [n_chars];
	for (int i = 0; i < n_chars; i++)
	{
		chars[i] = new Image8(12, 8,1);
		SubImage(*font, *chars[i], 8*i, 0);
	}
}

Font::~Font()
{
	delete font;
	for (int i = 0; i < n_chars; i++)
		delete chars[i];
	delete[] chars;
}

//NOTE Color must be an array that has the same size as the number of channels in the input image
void Font::DrawText(Image8 &image, const char *text, unsigned int tl_x, unsigned int tl_y, unsigned char *color)
{
	unsigned int len = strlen(text);

	for (unsigned int i = 0; i < len; i++)
	{
		unsigned int x = tl_x + i*8;
		unsigned int y = tl_y;

		if (x + 7 >= image.getWidth())
			break;
		if (y + 11 >= image.getHeight())
			break;

		for (unsigned int j = 0; j < 12; j++)
			for (unsigned int k = 0; k < 8; k++)
				if ((*chars[table[(unsigned int)text[i]]]).read(j,k,0))
				{
					for (unsigned int chanIter = 0; chanIter < image.getChannels(); chanIter++)
						image.write(y+j, x+k, chanIter,color[chanIter]);
				}
				else
				{
					for (unsigned int chanIter = 0; chanIter < image.getChannels(); chanIter++)
						image.write(y+j, x+k, chanIter,0);
				}
	}
}
