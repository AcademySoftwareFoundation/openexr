#include <ImfRgbaFile.h>
#include <ImfArray.h>

void drawImage1 (Imf::Array2D<Imf::Rgba> &pixels,
		 int width,
		 int height);

void drawImage2 (Imf::Array2D<half>  &gPixels,
		 Imf::Array2D<float> &zPixels,
		 int width,
		 int height);

void drawImage3 (Imf::Array2D<Imf::Rgba> &pixels,
                 int width,
                 int height,
                 int x_min, int x_max,
                 int y_min, int y_max,
                 int level = 0);

void drawImage4 (Imf::Array2D<Imf::Rgba> &pixels,
                 int width,
                 int height,
                 int x_min, int x_max,
                 int y_min, int y_max,
                 int level = 0);

void drawImage5 (Imf::Array2D<Imf::Rgba> &pixels,
                 int width,
                 int height,
                 int x_min, int x_max,
                 int y_min, int y_max,
                 int level = 0);

void drawImage6 (Imf::Array2D<half> &pixels, int w, int h);
