#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <iostream>

int
main()
{
    int width =  10;
    int height = 10;
        
    Imf::Array2D<Imf::Rgba> pixels(width, height);
    for (int y=0; y<height; y++)
        for (int x=0; x<width; x++)
            pixels[y][x] = Imf::Rgba(0, x / (width-1.0f), y / (height-1.0f));
        
    try {
        Imf::RgbaOutputFile file ("hello.exr", width, height, Imf::WRITE_RGBA);
        file.setFrameBuffer (&pixels[0][0], 1, width);
        file.writePixels (height);
    } catch (const std::exception &e) {
        std::cerr << "error writing image file hello.exr:" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
