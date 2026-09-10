//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "ImfArray.h"
#include "ImfHeader.h"
#include "ImfRgbaFile.h"
#include "ImfTiledRgbaFile.h"
#include "ImfCompression.h"

#include <Imath/ImathBox.h>

#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace std;

//
// LJ2K codes every chunk (or tile) as an independent, lossy JPEG 2000
// codestream.  Without the row / column replication done inside the
// compressor, an even chunk dimension leaves a visible discontinuity on the
// last row / column of every chunk (about 10 dB below the interior rows on a
// smooth gradient).  These tests write smooth gradients and check that the
// rows and columns adjacent to every chunk / tile boundary are coded about
// as well as the interior, and that odd chunk sizes round-trip.
//

namespace
{

double
gradient (int x, int y)
{
    return (60.0 + 0.25 * y + 30.0 * sin (x / 70.0)) / 255.0;
}

void
fillGradient (Array2D<Rgba>& px, int width, int height)
{
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            float v  = (float) gradient (x, y);
            px[y][x] = Rgba (v, v, v, 1.f);
        }
}

// PSNR of one row / column, on an 8-bit scale, against the gradient
double
rowPsnr (const Array2D<Rgba>& px, int width, int y)
{
    double e = 0;
    for (int x = 0; x < width; ++x)
    {
        double d = 255.0 * ((double) px[y][x].g - gradient (x, y));
        e += d * d;
    }
    return 10.0 * log10 (255.0 * 255.0 / (e / width));
}

double
colPsnr (const Array2D<Rgba>& px, int height, int x)
{
    double e = 0;
    for (int y = 0; y < height; ++y)
    {
        double d = 255.0 * ((double) px[y][x].g - gradient (x, y));
        e += d * d;
    }
    return 10.0 * log10 (255.0 * 255.0 / (e / height));
}

void
readBack (const string& fn, Array2D<Rgba>& px, int width, int height)
{
    RgbaInputFile in (fn.c_str ());
    Box2i         dw = in.dataWindow ();
    assert (dw.max.x - dw.min.x + 1 == width);
    assert (dw.max.y - dw.min.y + 1 == height);
    px.resizeErase (height, width);
    in.setFrameBuffer (&px[0][0], 1, width);
    in.readPixels (dw.min.y, dw.max.y);
}

// The last row / column of every chunk (the ones that the padding fixes;
// without it they are 10-15 dB below the interior on this gradient) must be
// within `tolerance` dB of the interior rows / columns.  The first row /
// column of the next chunk is printed for information: a small residual
// there is intrinsic to independent coding of the chunks and not affected
// by the padding.
void
checkBoundaries (
    const Array2D<Rgba>& px,
    int                  width,
    int                  height,
    int                  stepX,
    int                  stepY,
    double               tolerance)
{
    double interiorRow = rowPsnr (px, width, stepY / 2);
    double interiorCol = colPsnr (px, height, stepX / 2);

    cout << "   interior: row " << interiorRow << " dB, column " << interiorCol
         << " dB" << endl;

    for (int b = stepY; b <= height; b += stepY)
    {
        double before = rowPsnr (px, width, b - 1);
        cout << "   row " << b - 1 << ": " << before << " dB";
        if (b < height)
        {
            cout << ", row " << b << ": " << rowPsnr (px, width, b) << " dB";
        }
        cout << endl;
        assert (before > interiorRow - tolerance);
    }
    for (int b = stepX; b <= width; b += stepX)
    {
        double before = colPsnr (px, height, b - 1);
        cout << "   column " << b - 1 << ": " << before << " dB";
        if (b < width)
        {
            cout << ", column " << b << ": " << colPsnr (px, height, b)
                 << " dB";
        }
        cout << endl;
        assert (before > interiorCol - tolerance);
    }
    // the last row / column of the image are chunk boundaries too
    assert (rowPsnr (px, width, height - 1) > interiorRow - tolerance);
    assert (colPsnr (px, height, width - 1) > interiorCol - tolerance);
}

void
testScanlines (const string& tempDir, int width, int height, float quality)
{
    cout << "scanline " << width << "x" << height << ", quality " << quality
         << endl;

    string fn = tempDir + "imf_test_lj2k_boundary_scanline.exr";

    Array2D<Rgba> px (height, width);
    fillGradient (px, width, height);

    {
        Header hdr (width, height);
        hdr.compression ()       = LJ2K_COMPRESSION;
        hdr.lossyHTJ2KQuality () = quality;
        RgbaOutputFile out (fn.c_str (), hdr, WRITE_RGBA);
        out.setFrameBuffer (&px[0][0], 1, width);
        out.writePixels (height);
    }

    Array2D<Rgba> rd;
    readBack (fn, rd, width, height);
    checkBoundaries (rd, width, height, width, 256, 6.0);
    remove (fn.c_str ());
}

void
testTiles (
    const string& tempDir, int width, int height, int tileSize, float quality)
{
    cout << "tiled " << width << "x" << height << ", tiles " << tileSize
         << ", quality " << quality << endl;

    string fn = tempDir + "imf_test_lj2k_boundary_tiled.exr";

    Array2D<Rgba> px (height, width);
    fillGradient (px, width, height);

    {
        Header hdr (width, height);
        hdr.compression ()       = LJ2K_COMPRESSION;
        hdr.lossyHTJ2KQuality () = quality;
        TiledRgbaOutputFile out (
            fn.c_str (), hdr, WRITE_RGBA, tileSize, tileSize, ONE_LEVEL);
        out.setFrameBuffer (&px[0][0], 1, width);
        out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
    }

    Array2D<Rgba> rd;
    readBack (fn, rd, width, height);
    checkBoundaries (rd, width, height, tileSize, tileSize, 6.0);
    remove (fn.c_str ());
}

// Sizes around the padding modulus (2^5 = 32 for 5 decomposition levels)
// must round-trip; the padded codestream may be up to 31 rows / columns
// larger than the chunk.
void
testCornerSizes (const string& tempDir)
{
    const int sizes[] = {1, 2, 3, 31, 32, 33, 63, 64, 65};
    string    fn      = tempDir + "imf_test_lj2k_boundary_corner.exr";

    for (int width: sizes)
        for (int height: sizes)
        {
            Array2D<Rgba> px (height, width);
            fillGradient (px, width, height);
            {
                Header hdr (width, height);
                hdr.compression ()       = LJ2K_COMPRESSION;
                hdr.lossyHTJ2KQuality () = 60.f;
                RgbaOutputFile out (fn.c_str (), hdr, WRITE_RGBA);
                out.setFrameBuffer (&px[0][0], 1, width);
                out.writePixels (height);
            }
            Array2D<Rgba> rd;
            readBack (fn, rd, width, height);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    assert (
                        fabs ((double) rd[y][x].g - gradient (x, y)) < 0.05);
                    assert ((double) rd[y][x].a == 1.0);
                }
            remove (fn.c_str ());
        }
    cout << "corner sizes ok" << endl;
}

} // namespace

void
testLJ2KBoundary (const string& tempDir)
{
    try
    {
        cout << "Testing LJ2K chunk / tile boundaries" << endl;

        testScanlines (tempDir, 512, 512, 45.f);
        testScanlines (tempDir, 500, 300, 60.f);
        testTiles (tempDir, 512, 512, 64, 45.f);
        testTiles (tempDir, 500, 300, 64, 60.f);
        testCornerSizes (tempDir);

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
