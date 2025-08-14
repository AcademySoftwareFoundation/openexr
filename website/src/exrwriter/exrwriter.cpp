#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <iostream>

using namespace OPENEXR_IMF_NAMESPACE;

int
main()
{
    int width =  100;
    int height = 50;
        
    Array2D<Rgba> pixels(height, width);
    for (int y=0; y<height; y++)
    {
        float c = (y / 5 % 2 == 0) ? (y / (float) height) : 0.0;
        for (int x=0; x<width; x++)
            pixels[y][x] = Rgba(c, c, c);
    }

    try {
        RgbaOutputFile file ("stripes.exr", width, height, WRITE_RGBA);
        file.setFrameBuffer (&pixels[0][0], 1, width);
        file.writePixels (height);
    } catch (const std::exception &e) {
        std::cerr << "error writing image file stripes.exr:" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
