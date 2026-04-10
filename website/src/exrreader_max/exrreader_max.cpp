//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

static void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
              << " [--maxImageSize <width> <height>] <image.exr>\n";
}

int
main(int argc, char *argv[])
{
    int         maxW = 10000;
    int         maxH = 10000;
    std::string path;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--maxImageSize") == 0)
        {
            if (i + 2 >= argc)
            {
                usage(argv[0]);
                return 1;
            }
            try
            {
                maxW = std::stoi(argv[i + 1]);
                maxH = std::stoi(argv[i + 2]);
            }
            catch (const std::exception &)
            {
                std::cerr << "invalid width or height for --maxImageSize\n";
                return 1;
            }
            if (maxW <= 0 || maxH <= 0)
            {
                std::cerr << "--maxImageSize width and height must be positive\n";
                return 1;
            }
            i += 2;
        }
        else if (argv[i][0] == '-')
        {
            std::cerr << "unknown option: " << argv[i] << std::endl;
            usage(argv[0]);
            return 1;
        }
        else
        {
            if (!path.empty())
            {
                std::cerr << "unexpected extra argument: " << argv[i]
                          << std::endl;
                usage(argv[0]);
                return 1;
            }
            path = argv[i];
        }
    }

    if (path.empty())
    {
        usage(argv[0]);
        return 1;
    }

    Imf::Header::setMaxImageSize(maxW, maxH);

    try
    {
        Imf::RgbaInputFile file(path.c_str());
        Imath::Box2i       dw = file.dataWindow();
        int                width  = dw.max.x - dw.min.x + 1;
        int                height = dw.max.y - dw.min.y + 1;

        Imf::Array2D<Imf::Rgba> pixels(width, height);

        file.setFrameBuffer(&pixels[0][0], 1, width);
        file.readPixels(dw.min.y, dw.max.y);
    }
    catch (const std::exception &e)
    {
        std::cerr << "error reading image file " << path << ": " << e.what()
                  << std::endl;
        return 1;
    }

    return 0;
}
