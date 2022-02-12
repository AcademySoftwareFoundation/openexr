//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "IlmThread.h"
#include <ImfArray.h>
#include <ImfThreading.h>
#include <ImfTiledRgbaFile.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

void
waves (Array2D<Rgba>& pixels, int w, int h)
{
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];

            p.r = (0.5 + 0.5 * sin (0.1 * x + 0.1 * y)) +
                  (0.5 + 0.5 * sin (-0.1 * x + 0.2 * y));

            p.g = p.r;
            p.b = p.r;

            p.a = (0.5 + 0.5 * sin (1.1 * x + 0.5 * y));
        }
    }
}

void
wheel (Array2D<Rgba>& pixels, int w, int h)
{
    float n      = 40;
    float m      = 0.5;
    float radMin = 2 * n / M_PI;
    float xCen   = w * 0.5;
    float yCen   = h * 0.5;
    float radMax = (xCen < yCen) ? xCen : yCen;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Rgba& p = pixels[y][x];
            float rad =
                sqrt ((x - xCen) * (x - xCen) + (y - yCen) * (y - yCen));

            if (rad <= radMax && rad >= radMin)
            {
                float phi = atan2 (y - yCen, x - xCen);
                float c   = 0.5 + 0.5 * sin (phi * n + rad * m);

                p.r = 0.5 + 0.5 * c;
                p.g = p.r;
                p.b = p.r;
                p.a = 0.5 - 0.5 * c;
            }
            else
            {
                p.r = 0.5;
                p.g = p.r;
                p.b = p.r;
                p.a = p.r;
            }
        }
    }
}

void
writeReadYa (
    Box2i&     dw,
    int        tileSizeX,
    int        tileSizeY,
    const char fileName[],
    void (*fillPixels) (Array2D<Rgba>& pixels, int w, int h))
{
    int           w = dw.max.x - dw.min.x + 1;
    int           h = dw.max.y - dw.min.y + 1;
    Array2D<Rgba> pixels1 (h, w);
    Array2D<Rgba> pixels2 (h, w);

    fillPixels (pixels1, w, h);

    cout << "writing " << flush;

    {
        TiledRgbaOutputFile out (
            fileName,
            tileSizeX,
            tileSizeY,
            ONE_LEVEL,
            ROUND_DOWN,
            dw,
            dw,
            WRITE_YA);

        out.setFrameBuffer (&pixels1[-dw.min.y][-dw.min.x], 1, w);
        out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
    }

    cout << "reading " << flush;

    {
        TiledRgbaInputFile in (fileName);

        in.setFrameBuffer (&pixels2[-dw.min.y][-dw.min.x], 1, w);
        in.readTiles (0, in.numXTiles () - 1, 0, in.numYTiles () - 1);
    }

    cout << "comparing" << endl;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Rgba& p1 = pixels1[y][x];
            const Rgba& p2 = pixels2[y][x];

            assert (p1.r == p2.r);
            assert (p1.g == p2.g);
            assert (p1.b == p2.b);
            assert (p1.a == p2.a);
        }
    }

    remove (fileName);
}

} // namespace

void
testTiledYa (const std::string& tempDir)
{
    try
    {
        cout << "Testing tiled luminance input and output" << endl;

        std::string fileName = tempDir + "imf_test_tiled_ya.exr";

        int maxThreads = ILMTHREAD_NAMESPACE::supportsThreads () ? 3 : 0;

        for (int n = 0; n <= maxThreads; ++n)
        {
            if (ILMTHREAD_NAMESPACE::supportsThreads ())
            {
                setGlobalThreadCount (n);
                cout << "\nnumber of threads: " << globalThreadCount () << endl;
            }

            Box2i dataWindow (V2i (-17, -29), V2i (348, 556));
            writeReadYa (dataWindow, 19, 27, fileName.c_str (), waves);
            writeReadYa (dataWindow, 19, 27, fileName.c_str (), wheel);
        }

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
