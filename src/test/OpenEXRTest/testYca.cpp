//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "IlmThread.h"
#include "ImathMath.h"
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfThreading.h>
#include <algorithm>
#include <assert.h>
#include <stdio.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

void
fillPixelsColor (Array2D<Rgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];

            p.r = 0.8 + 0.5 * sin (x * 0.05);
            p.g = 0.8 + 0.5 * sin (x * 0.02 + y * 0.02);
            p.b = 0.8 + 0.5 * sin (y * 0.03);

            float t = 0.8 + 0.5 * sin (x * 0.05 - y * 0.05);

            p.r *= t;
            p.g *= t;
            p.b *= t;
            p.a = t;
        }
    }
}

void
fillPixelsGray (Array2D<Rgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];

            p.r = 0.8 + 0.5 * sin (x * 0.05 - y * 0.05);
            p.g = p.r;
            p.b = p.r;
            p.a = 0.5 + 0.5 * cos (x * 0.05 - y * 0.05);
        }
    }
}

void
writeReadYca (
    const char   fileName[],
    const Box2i& dw,
    RgbaChannels channels,
    LineOrder    writeOrder,
    LineOrder    readOrder,
    void (*fillPixels) (Array2D<Rgba>& pixels, int w, int h))
{
    int           w = dw.max.x - dw.min.x + 1;
    int           h = dw.max.y - dw.min.y + 1;
    Array2D<Rgba> pixels1 (h, w);
    Array2D<Rgba> pixels2 (h, w);

    cout << w << " by " << h
         << " pixels, "
            "channels "
         << channels
         << ", "
            "write order "
         << writeOrder
         << ", "
            "read order "
         << readOrder << endl;

    fillPixels (pixels1, w, h);

    cout << "writing " << flush;

    {
        RgbaOutputFile out (
            fileName,
            dw,
            dw, // display window, data window
            channels,
            1,          // pixelAspectRatio
            V2f (0, 0), // screenWindowCenter
            1,          // screenWindowWidth
            writeOrder);

        out.setYCRounding (9, 9);
        out.setFrameBuffer (&pixels1[-dw.min.y][-dw.min.x], 1, w);
        out.writePixels (h);
    }

    cout << "reading " << flush;

    {
        RgbaInputFile in (fileName);

        in.setFrameBuffer (&pixels2[-dw.min.y][-dw.min.x], 1, w);

        switch (readOrder)
        {
            case INCREASING_Y:

                for (int y = dw.min.y; y <= dw.max.y; ++y)
                    in.readPixels (y);

                break;

            case DECREASING_Y:

                for (int y = dw.max.y; y >= dw.min.y; --y)
                    in.readPixels (y);

                break;

            case RANDOM_Y:

                assert (h % 5 != 0);

                for (int i = 0; i < h; ++i)
                {
                    int y = dw.min.y + (i * 5) % h;
                    in.readPixels (y);
                }

                break;
            case NUM_LINEORDERS:
            default:
                cerr << "invalid line order " << int (readOrder) << std::endl;
                break;
        }
    }

    cout << "comparing" << endl;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Rgba& p1 = pixels1[y][x];
            const Rgba& p2 = pixels2[y][x];

            if (channels & WRITE_C)
            {
                float p1Max = max (p1.r, max (p1.g, p1.b));
                float p2Max = max (p2.r, max (p2.g, p2.b));

                assert (equalWithAbsError (p1Max, p2Max, 0.03f));
            }
            else
            {
                assert (p1.g == p2.g);
                assert (p1.b == p2.b);
            }

            if (channels & WRITE_A) { assert (p1.a == p2.a); }
        }
    }

    remove (fileName);
}

} // namespace

void
testYca (const std::string& tempDir)
{
    try
    {
        cout << "Testing luminance/chroma input and output" << endl;

        std::string fileName = tempDir + "imf_test_yca.exr";

        Box2i dataWindow[6];
        dataWindow[0] = Box2i (V2i (0, 0), V2i (1, 17));
        dataWindow[1] = Box2i (V2i (0, 0), V2i (5, 17));
        dataWindow[2] = Box2i (V2i (0, 0), V2i (17, 1));
        dataWindow[3] = Box2i (V2i (0, 0), V2i (17, 5));
        dataWindow[4] = Box2i (V2i (0, 0), V2i (1, 1));
        dataWindow[5] = Box2i (V2i (-18, -28), V2i (247, 255));

        int maxThreads = ILMTHREAD_NAMESPACE::supportsThreads () ? 3 : 0;

        for (int n = 0; n <= maxThreads; ++n)
        {
            if (ILMTHREAD_NAMESPACE::supportsThreads ())
            {
                setGlobalThreadCount (n);
                cout << "\nnumber of threads: " << globalThreadCount () << endl;
            }

            for (int i = 0; i < 6; ++i)
            {
                for (int writeOrder = INCREASING_Y; writeOrder <= DECREASING_Y;
                     ++writeOrder)
                {
                    for (int readOrder = INCREASING_Y; readOrder <= RANDOM_Y;
                         ++readOrder)
                    {
                        writeReadYca (
                            fileName.c_str (),
                            dataWindow[i],
                            WRITE_YCA,
                            LineOrder (writeOrder),
                            LineOrder (readOrder),
                            fillPixelsColor);

                        writeReadYca (
                            fileName.c_str (),
                            dataWindow[i],
                            WRITE_YC,
                            LineOrder (writeOrder),
                            LineOrder (readOrder),
                            fillPixelsColor);

                        writeReadYca (
                            fileName.c_str (),
                            dataWindow[i],
                            WRITE_YA,
                            LineOrder (writeOrder),
                            LineOrder (readOrder),
                            fillPixelsGray);

                        writeReadYca (
                            fileName.c_str (),
                            dataWindow[i],
                            WRITE_Y,
                            LineOrder (writeOrder),
                            LineOrder (readOrder),
                            fillPixelsGray);
                    }
                }
            }
        }

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
