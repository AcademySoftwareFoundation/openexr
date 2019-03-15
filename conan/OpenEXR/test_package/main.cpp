#include <iostream>
#include <cstdlib>

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <half.h>

int main (int argc, char *argv[])
{
    if (argc < 2)
        return EXIT_FAILURE;
    Imf::RgbaInputFile input_file(argv[1]);
    const Imf::Header& header = input_file.header();
    size_t width = header.dataWindow().max.x - header.dataWindow().min.x + 1;
    size_t height = header.dataWindow().max.y - header.dataWindow().min.y + 1;
    std::cout << "OpenEXR images size is " << width << "x" << height << std::endl;
    return EXIT_SUCCESS;
}
