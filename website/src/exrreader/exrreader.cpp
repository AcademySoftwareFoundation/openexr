#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <iostream>

int
main()
{
    try {
        Imf::RgbaInputFile file("hello.exr");
        Imath::Box2i       dw = file.dataWindow();
        int                width  = dw.max.x - dw.min.x + 1;
        int                height = dw.max.y - dw.min.y + 1;

        Imf::Array2D<Imf::Rgba> pixels(width, height);
        
        file.setFrameBuffer(&pixels[0][0], 1, width);
        file.readPixels(dw.min.y, dw.max.y);

    } catch (const std::exception &e) {
        std::cerr << "error reading image file hello.exr:" << e.what() << std::endl;
        return 1;
    }

    return 0;
}
