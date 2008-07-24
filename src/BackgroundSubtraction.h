#ifndef BACKGROUND_SUBTRACTION_H
#define BACKGROUND_SUBTRACTION_H
#include "imageClass/Image8.h"
class BackgroundSubtraction
{
public:
	virtual void subtract (Image8 &differenceImage, Image8 &currentImage) = 0;
	virtual void hint(const char * name, Image8 &hintImage) = 0;/* Hints are a way of sending higher level orders through the interface to the algoritihms, they may be ignored without issue */
	virtual void hint(const char * name, double hintValue) = 0;
	virtual void resetHints() = 0;//Called to clear the underlying hints
	virtual void getBackgroundImage(Image8 &backgroundImage) = 0;//This will return an image representing color values that make up to the background, implementations should make it the MOST probable background
	virtual ~BackgroundSubtraction()
	{}
}
;
#endif
