#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <iostream>

int
main()
{
    int width =  20;
    int height = 10;
        
    Imf::Array2D<Imf::Rgba> pixels(width, height);
    for (int x=0; x<width; x++)
        for (int y=0; y<height; y++)
            pixels[x][y] = Imf::Rgba(0, x / (width-1.0f), y / (height-1.0f));
        
    try {
        Imf::RgbaOutputFile file ("hello.exr", width, height, Imf::WRITE_RGBA);
        file.setFrameBuffer (&pixels[0][0], height, 1);
        file.writePixels (height);
    } catch (const std::exception &e) {
        std::cerr << "error writing image file hello.exr:" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
