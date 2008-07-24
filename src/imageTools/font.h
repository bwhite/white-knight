#ifndef FONT_H
#define FONT_H
#include "../imageClass/Image8.h"
class Font
{
public:
	Font();
	~Font();

	void DrawText(Image8 &image, const char *text, unsigned int tl_x, unsigned int tl_y, unsigned char *color);
private:

	Image8 *font;
	Image8 **chars;
	int n_chars;

	int table[256];
};
#endif
