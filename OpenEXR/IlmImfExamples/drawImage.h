#include <ImfRgbaFile.h>
#include <ImfArray.h>

void drawImage1 (Imf::Array2D<Imf::Rgba> &pixels,
		 int width,
		 int height);

void drawImage2 (Imf::Array2D<half>  &gPixels,
		 Imf::Array2D<float> &zPixels,
		 int width,
		 int height);
