#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <iostream>

int
main()
{
    int width =  100;
    int height = 50;
        
    Imf::Array2D<Imf::Rgba> pixels(height, width);
    for (int y=0; y<height; y++)
    {
        float c = (y / 5 % 2 == 0) ? (y / (float) height) : 0.0;
        for (int x=0; x<width; x++)
            pixels[y][x] = Imf::Rgba(c, c, c);
    }

    try {
        Imf::RgbaOutputFile file ("stripes.exr", width, height, Imf::WRITE_RGBA);
        file.setFrameBuffer (&pixels[0][0], 1, width);
        file.writePixels (height);
    } catch (const std::exception &e) {
        std::cerr << "error writing image file stripes.exr:" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
