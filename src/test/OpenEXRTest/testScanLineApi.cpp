//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "IlmThread.h"

#include "Iex.h"

#include "ImfArray.h"
#include "ImfChannelList.h"
#include "ImfInputFile.h"
#include "ImfRgbaFile.h"
#include "ImfThreading.h"
#include "ImfTileDescriptionAttribute.h"
#include "ImfTiledInputFile.h"
#include "ImfTiledOutputFile.h"
#include "ImfTiledRgbaFile.h"
#include "ImfOutputFile.h"
#include "ImfFrameBuffer.h"
#include "ImfStdIO.h"
#include "ImfHeader.h"

#include <Imath/ImathRandom.h>

#include <algorithm>
#include <iostream>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

void
fillPixels (
    Array2D<unsigned int>& pi,
    Array2D<half>&         ph,
    Array2D<float>&        pf,
    int                    width,
    int                    height)
{
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            pi[y][x] = x % 100 + 100 * (y % 100);
            ph[y][x] = sin (double (x)) + sin (y * 0.5);
            pf[y][x] = sin (double (y)) + sin (x * 0.5);
        }
}

void
writeRead (
    const Array2D<unsigned int>& pi1,
    const Array2D<half>&         ph1,
    const Array2D<float>&        pf1,
    const char                   fileName[],
    LineOrder                    lorder,
    int                          width,
    int                          height,
    int                          xSize,
    int                          ySize,
    int                          xOffset,
    int                          yOffset,
    Compression                  comp,
    LevelMode                    mode,
    LevelRoundingMode            rmode,
    bool                         fillChannel)
{
    //
    // Write the pixel data in pi1, ph1 and ph2 to a tiled
    // image file using the specified parameters.
    // Read the pixel data back from the file using the scanline
    // interface one scanline at a time, and verify that the data did
    // not change.
    // For MIPMAP and RIPMAP_LEVELS, the lower levels of the images
    // are filled in cropped versions of the level(0,0) image,
    // i.e. no filtering is done.
    //

    cout << "levelMode " << mode << ", roundingMode " << rmode
         << ", line order " << lorder << ",\ntileSize " << xSize << "x" << ySize
         << ", xOffset " << xOffset << ", yOffset " << yOffset << endl;

    Header hdr (
        (Box2i (
            V2i (0, 0), // display window
            V2i (width - 1, height - 1))),
        (Box2i (
            V2i (xOffset, yOffset), // data window
            V2i (xOffset + width - 1, yOffset + height - 1))));
    hdr.lineOrder ()   = lorder;
    hdr.compression () = comp;

    hdr.channels ().insert ("I", Channel (IMF::UINT));
    hdr.channels ().insert ("H", Channel (IMF::HALF));
    hdr.channels ().insert ("F", Channel (IMF::FLOAT));

    hdr.setTileDescription (TileDescription (xSize, ySize, mode, rmode));
    {
        FrameBuffer fb;

        fb.insert (
            "I", // name
            Slice (
                IMF::UINT,                        // type
                (char*) &pi1[-yOffset][-xOffset], // base
                sizeof (pi1[0][0]),               // xStride
                sizeof (pi1[0][0]) * width)       // yStride
        );

        fb.insert (
            "H", // name
            Slice (
                IMF::HALF,                        // type
                (char*) &ph1[-yOffset][-xOffset], // base
                sizeof (ph1[0][0]),               // xStride
                sizeof (ph1[0][0]) * width)       // yStride
        );

        fb.insert (
            "F", // name
            Slice (
                IMF::FLOAT,                       // type
                (char*) &pf1[-yOffset][-xOffset], // base
                sizeof (pf1[0][0]),               // xStride
                sizeof (pf1[0][0]) * width)       // yStride
        );

        cout << " writing" << flush;

        remove (fileName);
        TiledOutputFile out (fileName, hdr);
        out.setFrameBuffer (fb);

        int startTileY = -1;
        int endTileY   = -1;
        int dy;

        switch (mode)
        {
            case ONE_LEVEL: {
                if (lorder == DECREASING_Y)
                {
                    startTileY = out.numYTiles () - 1;
                    endTileY   = -1;

                    dy = -1;
                }
                else
                {
                    startTileY = 0;
                    endTileY   = out.numYTiles ();

                    dy = 1;
                }

                for (int tileY = startTileY; tileY != endTileY; tileY += dy)
                    for (int tileX = 0; tileX < out.numXTiles (); ++tileX)
                        out.writeTile (tileX, tileY);
            }
            break;

            case MIPMAP_LEVELS: {
                if (lorder == DECREASING_Y)
                {
                    endTileY = -1;
                    dy       = -1;
                }
                else
                {
                    startTileY = 0;
                    dy         = 1;
                }

                for (int level = 0; level < out.numLevels (); ++level)
                {
                    if (lorder == DECREASING_Y)
                        startTileY = out.numYTiles (level) - 1;
                    else
                        endTileY = out.numYTiles (level);

                    for (int tileY = startTileY; tileY != endTileY; tileY += dy)
                        for (int tileX = 0; tileX < out.numXTiles (level);
                             ++tileX)
                            out.writeTile (tileX, tileY, level);
                }
            }
            break;

            case RIPMAP_LEVELS: {
                for (int ylevel = 0; ylevel < out.numYLevels (); ++ylevel)
                {
                    if (lorder == DECREASING_Y)
                    {
                        startTileY = out.numYTiles (ylevel) - 1;
                        endTileY   = -1;

                        dy = -1;
                    }
                    else
                    {
                        startTileY = 0;
                        endTileY   = out.numYTiles (ylevel);

                        dy = 1;
                    }

                    for (int xlevel = 0; xlevel < out.numXLevels (); ++xlevel)
                    {
                        for (int tileY = startTileY; tileY != endTileY;
                             tileY += dy)
                            for (int tileX = 0; tileX < out.numXTiles (xlevel);
                                 ++tileX)
                                out.writeTile (tileX, tileY, xlevel, ylevel);
                    }
                }
            }
            break;
            case NUM_LEVELMODES:
            default:
                std::cerr << "Invalid tile mode " << int (mode) << std::endl;
                break;
        }
    }

    {
        cout << " reading INCREASING_Y" << flush;

        InputFile in (fileName);

        const Box2i& dw  = in.header ().dataWindow ();
        int          w   = dw.max.x - dw.min.x + 1;
        int          h   = dw.max.y - dw.min.y + 1;
        int          dwx = dw.min.x;
        int          dwy = dw.min.y;

        Array2D<unsigned int> pi2 (h, w);
        Array2D<half>         ph2 (h, w);
        Array2D<float>        pf2 (h, w);

        Array2D<unsigned int> fi2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<half>         fh2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<float>        ff2 (fillChannel ? h : 1, fillChannel ? w : 1);

        const unsigned int fillInt   = 12;
        const half         fillHalf  = 4.5;
        const float        fillFloat = M_PI;

        FrameBuffer fb;

        fb.insert (
            "I", // name
            Slice (
                IMF::UINT,                // type
                (char*) &pi2[-dwy][-dwx], // base
                sizeof (pi2[0][0]),       // xStride
                sizeof (pi2[0][0]) * w)   // yStride
        );

        fb.insert (
            "H", // name
            Slice (
                IMF::HALF,                // type
                (char*) &ph2[-dwy][-dwx], // base
                sizeof (ph2[0][0]),       // xStride
                sizeof (ph2[0][0]) * w)   // yStride
        );

        fb.insert (
            "F", // name
            Slice (
                IMF::FLOAT,               // type
                (char*) &pf2[-dwy][-dwx], // base
                sizeof (pf2[0][0]),       // xStride
                sizeof (pf2[0][0]) * w)   // yStride
        );

        if (fillChannel)
        {
            fb.insert (
                "FI", // name
                Slice (
                    IMF::UINT,                // type
                    (char*) &fi2[-dwy][-dwx], // base
                    sizeof (fi2[0][0]),       // xStride
                    sizeof (fi2[0][0]) * w,
                    1,
                    1,
                    fillInt) // yStride
            );

            fb.insert (
                "FH", // name
                Slice (
                    IMF::HALF,                // type
                    (char*) &fh2[-dwy][-dwx], // base
                    sizeof (fh2[0][0]),       // xStride
                    sizeof (fh2[0][0]) * w,
                    1,
                    1,
                    fillHalf) // yStride
            );

            fb.insert (
                "FF", // name
                Slice (
                    IMF::FLOAT,               // type
                    (char*) &ff2[-dwy][-dwx], // base
                    sizeof (ff2[0][0]),       // xStride
                    sizeof (ff2[0][0]) * w,
                    1,
                    1,
                    fillFloat) // yStride
            );
        }

        in.setFrameBuffer (fb);
        for (int y = dw.min.y; y <= dw.max.y; ++y)
            in.readPixels (y);

        cout << " comparing" << flush;

        assert (in.header ().displayWindow () == hdr.displayWindow ());
        assert (in.header ().dataWindow () == hdr.dataWindow ());
        assert (in.header ().pixelAspectRatio () == hdr.pixelAspectRatio ());
        assert (
            in.header ().screenWindowCenter () == hdr.screenWindowCenter ());
        assert (in.header ().screenWindowWidth () == hdr.screenWindowWidth ());
        assert (in.header ().lineOrder () == hdr.lineOrder ());
        assert (in.header ().compression () == hdr.compression ());

        ChannelList::ConstIterator hi = hdr.channels ().begin ();
        ChannelList::ConstIterator ii = in.header ().channels ().begin ();

        while (hi != hdr.channels ().end ())
        {
            assert (!strcmp (hi.name (), ii.name ()));
            assert (hi.channel ().type == ii.channel ().type);
            assert (hi.channel ().xSampling == ii.channel ().xSampling);
            assert (hi.channel ().ySampling == ii.channel ().ySampling);

            ++hi;
            ++ii;
        }

        assert (ii == in.header ().channels ().end ());

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                assert (pi1[y][x] == pi2[y][x]);
                assert (ph1[y][x] == ph2[y][x]);
                assert (pf1[y][x] == pf2[y][x]);

                if (fillChannel)
                {
                    assert (fi2[y][x] == fillInt);
                    assert (fh2[y][x] == fillHalf);
                    assert (ff2[y][x] == fillFloat);
                }
            }
        }
    }

    {
        cout << endl << "         reading DECREASING_Y" << flush;

        InputFile in (fileName);

        const Box2i& dw  = in.header ().dataWindow ();
        int          w   = dw.max.x - dw.min.x + 1;
        int          h   = dw.max.y - dw.min.y + 1;
        int          dwx = dw.min.x;
        int          dwy = dw.min.y;

        Array2D<unsigned int> pi2 (h, w);
        Array2D<half>         ph2 (h, w);
        Array2D<float>        pf2 (h, w);

        Array2D<unsigned int> fi2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<half>         fh2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<float>        ff2 (fillChannel ? h : 1, fillChannel ? w : 1);

        FrameBuffer fb;

        fb.insert (
            "I", // name
            Slice (
                IMF::UINT,                // type
                (char*) &pi2[-dwy][-dwx], // base
                sizeof (pi2[0][0]),       // xStride
                sizeof (pi2[0][0]) * w)   // yStride
        );

        fb.insert (
            "H", // name
            Slice (
                IMF::HALF,                // type
                (char*) &ph2[-dwy][-dwx], // base
                sizeof (ph2[0][0]),       // xStride
                sizeof (ph2[0][0]) * w)   // yStride
        );

        fb.insert (
            "F", // name
            Slice (
                IMF::FLOAT,               // type
                (char*) &pf2[-dwy][-dwx], // base
                sizeof (pf2[0][0]),       // xStride
                sizeof (pf2[0][0]) * w)   // yStride
        );
        const unsigned int fillInt   = 21;
        const half         fillHalf  = 42;
        const float        fillFloat = 2.8;

        if (fillChannel)
        {
            fb.insert (
                "FI", // name
                Slice (
                    IMF::UINT,                // type
                    (char*) &fi2[-dwy][-dwx], // base
                    sizeof (fi2[0][0]),       // xStride
                    sizeof (fi2[0][0]) * w,
                    1,
                    1,
                    fillInt) // yStride
            );

            fb.insert (
                "FH", // name
                Slice (
                    IMF::HALF,                // type
                    (char*) &fh2[-dwy][-dwx], // base
                    sizeof (fh2[0][0]),       // xStride
                    sizeof (fh2[0][0]) * w,
                    1,
                    1,
                    fillHalf) // yStride
            );

            fb.insert (
                "FF", // name
                Slice (
                    IMF::FLOAT,               // type
                    (char*) &ff2[-dwy][-dwx], // base
                    sizeof (ff2[0][0]),       // xStride
                    sizeof (ff2[0][0]) * w,
                    1,
                    1,
                    fillFloat) // yStride
            );
        }

        in.setFrameBuffer (fb);
        for (int y = dw.max.y; y >= dw.min.y; --y)
            in.readPixels (y);

        cout << " comparing" << flush;

        assert (in.header ().displayWindow () == hdr.displayWindow ());
        assert (in.header ().dataWindow () == hdr.dataWindow ());
        assert (in.header ().pixelAspectRatio () == hdr.pixelAspectRatio ());
        assert (
            in.header ().screenWindowCenter () == hdr.screenWindowCenter ());
        assert (in.header ().screenWindowWidth () == hdr.screenWindowWidth ());
        assert (in.header ().lineOrder () == hdr.lineOrder ());
        assert (in.header ().compression () == hdr.compression ());

        ChannelList::ConstIterator hi = hdr.channels ().begin ();
        ChannelList::ConstIterator ii = in.header ().channels ().begin ();

        while (hi != hdr.channels ().end ())
        {
            assert (!strcmp (hi.name (), ii.name ()));
            assert (hi.channel ().type == ii.channel ().type);
            assert (hi.channel ().xSampling == ii.channel ().xSampling);
            assert (hi.channel ().ySampling == ii.channel ().ySampling);

            ++hi;
            ++ii;
        }

        assert (ii == in.header ().channels ().end ());

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                assert (pi1[y][x] == pi2[y][x]);
                assert (ph1[y][x] == ph2[y][x]);
                assert (pf1[y][x] == pf2[y][x]);
                if (fillChannel)
                {
                    assert (fi2[y][x] == fillInt);
                    assert (fh2[y][x] == fillHalf);
                    assert (ff2[y][x] == fillFloat);
                }
            }
        }
    }

    {
        cout << endl
             << "         reading INCREASING_Y "
                "(new frame buffer on every line)"
             << flush;

        InputFile in (fileName);

        const Box2i& dw  = in.header ().dataWindow ();
        int          w   = dw.max.x - dw.min.x + 1;
        int          h   = dw.max.y - dw.min.y + 1;
        int          dwx = dw.min.x;
        int          dwy = dw.min.y;

        Array2D<unsigned int> pi2 (h, w);
        Array2D<half>         ph2 (h, w);
        Array2D<float>        pf2 (h, w);

        Array2D<unsigned int> fi2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<half>         fh2 (fillChannel ? h : 1, fillChannel ? w : 1);
        Array2D<float>        ff2 (fillChannel ? h : 1, fillChannel ? w : 1);

        const unsigned int fillInt   = 81;
        const half         fillHalf  = 0.5;
        const float        fillFloat = 7.8;

        for (int y = dw.min.y; y <= dw.max.y; ++y)
        {
            FrameBuffer fb;

            fb.insert (
                "I", // name
                Slice (
                    IMF::UINT,                   // type
                    (char*) &pi2[y - dwy][-dwx], // base
                    sizeof (pi2[0][0]),          // xStride
                    0)                           // yStride
            );

            fb.insert (
                "H", // name
                Slice (
                    IMF::HALF,                   // type
                    (char*) &ph2[y - dwy][-dwx], // base
                    sizeof (ph2[0][0]),          // xStride
                    0)                           // yStride
            );

            fb.insert (
                "F", // name
                Slice (
                    IMF::FLOAT,                  // type
                    (char*) &pf2[y - dwy][-dwx], // base
                    sizeof (pf2[0][0]),          // xStride
                    0)                           // yStride
            );

            if (fillChannel)
            {
                fb.insert (
                    "FI", // name
                    Slice (
                        IMF::UINT,                   // type
                        (char*) &fi2[y - dwy][-dwx], // base
                        sizeof (fi2[0][0]),          // xStride
                        0,
                        1,
                        1,
                        fillInt) // yStride
                );

                fb.insert (
                    "FH", // name
                    Slice (
                        IMF::HALF,                   // type
                        (char*) &fh2[y - dwy][-dwx], // base
                        sizeof (fh2[0][0]),          // xStride
                        0,
                        1,
                        1,
                        fillHalf) // yStride
                );

                fb.insert (
                    "FF", // name
                    Slice (
                        IMF::FLOAT,                  // type
                        (char*) &ff2[y - dwy][-dwx], // base
                        sizeof (ff2[0][0]),          // xStride
                        0,
                        1,
                        1,
                        fillFloat) // yStride
                );
            }

            in.setFrameBuffer (fb);
            in.readPixels (y);
        }

        cout << " comparing" << flush;

        assert (in.header ().displayWindow () == hdr.displayWindow ());
        assert (in.header ().dataWindow () == hdr.dataWindow ());
        assert (in.header ().pixelAspectRatio () == hdr.pixelAspectRatio ());
        assert (
            in.header ().screenWindowCenter () == hdr.screenWindowCenter ());
        assert (in.header ().screenWindowWidth () == hdr.screenWindowWidth ());
        assert (in.header ().lineOrder () == hdr.lineOrder ());
        assert (in.header ().compression () == hdr.compression ());

        ChannelList::ConstIterator hi = hdr.channels ().begin ();
        ChannelList::ConstIterator ii = in.header ().channels ().begin ();

        while (hi != hdr.channels ().end ())
        {
            assert (!strcmp (hi.name (), ii.name ()));
            assert (hi.channel ().type == ii.channel ().type);
            assert (hi.channel ().xSampling == ii.channel ().xSampling);
            assert (hi.channel ().ySampling == ii.channel ().ySampling);

            ++hi;
            ++ii;
        }

        assert (ii == in.header ().channels ().end ());

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                assert (pi1[y][x] == pi2[y][x]);
                assert (ph1[y][x] == ph2[y][x]);
                assert (pf1[y][x] == pf2[y][x]);
                if (fillChannel)
                {
                    assert (fi2[y][x] == fillInt);
                    assert (fh2[y][x] == fillHalf);
                    assert (ff2[y][x] == fillFloat);
                }
            }
        }
    }

    remove (fileName);
    cout << endl;
}

void
writeRead (
    const std::string&           tempDir,
    const Array2D<unsigned int>& pi,
    const Array2D<half>&         ph,
    const Array2D<float>&        pf,
    int                          W,
    int                          H,
    LineOrder                    lorder,
    Compression                  comp,
    LevelRoundingMode            rmode,
    int                          dx,
    int                          dy,
    int                          xSize,
    int                          ySize)
{
    std::string filename = tempDir + "imf_test_scanline_api.exr";

    writeRead (
        pi,
        ph,
        pf,
        filename.c_str (),
        lorder,
        W,
        H,
        xSize,
        ySize,
        dx,
        dy,
        comp,
        ONE_LEVEL,
        rmode,
        false);
    writeRead (
        pi,
        ph,
        pf,
        filename.c_str (),
        lorder,
        W,
        H,
        xSize,
        ySize,
        dx,
        dy,
        comp,
        MIPMAP_LEVELS,
        rmode,
        false);
    writeRead (
        pi,
        ph,
        pf,
        filename.c_str (),
        lorder,
        W,
        H,
        xSize,
        ySize,
        dx,
        dy,
        comp,
        RIPMAP_LEVELS,
        rmode,
        false);
    writeRead (
        pi,
        ph,
        pf,
        filename.c_str (),
        lorder,
        W,
        H,
        xSize,
        ySize,
        dx,
        dy,
        comp,
        ONE_LEVEL,
        rmode,
        true);
}

void
testTileCoordsFill (const std::string& tempDir)
{
    //
    // Read a tiled file through the scan line API into slices with
    // yTileCoords set, sized for a single row of tiles. Channels that
    // are not in the file ("Z") must be filled at the same tile-relative
    // rows as channels that are ("H"). The data window is walked in
    // ranges of 1 to TY scan lines, forwards and backwards, so ranges
    // straddle rows of tiles and the first line accessed in a row of
    // tiles isn't always its topmost line.
    //

    cout << "Testing yTileCoords fill slices on a tiled file" << endl;

    std::string fn = tempDir + "imf_test_tile_coords_fill.exr";

    const int W  = 16;
    const int H  = 12;
    const int DX = -3;
    const int DY = 5;
    const int TX = 8;
    const int TY = 4;

    Array2D<half> ph (H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            ph[y][x] = half (float (y * W + x));

    {
        Header hdr (
            Box2i (V2i (0, 0), V2i (W - 1, H - 1)),
            Box2i (V2i (DX, DY), V2i (DX + W - 1, DY + H - 1)));
        hdr.channels ().insert ("H", Channel (IMF::HALF));
        hdr.setTileDescription (TileDescription (TX, TY, ONE_LEVEL));

        TiledOutputFile out (fn.c_str (), hdr);
        FrameBuffer     fb;
        fb.insert (
            "H",
            Slice (
                IMF::HALF,
                (char*) &ph[-DY][-DX],
                sizeof (ph[0][0]),
                sizeof (ph[0][0]) * W));
        out.setFrameBuffer (fb);
        out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
    }

    {
        Array2D<half> hrow (TY, W);
        Array2D<half> zrow (TY, W);

        const half fillZ = 3.5;

        //
        // A range of at most TY lines lands on distinct rows of the
        // one-tile-row buffers, so after each readPixels call every
        // buffer row that was touched must hold the line it was given
        // and the other rows must be unchanged.
        //

        Array2D<half> expectH (TY, W);
        Array2D<half> expectZ (TY, W);

        for (int backwards = 0; backwards < 2; ++backwards)
        {
            for (int step = 1; step <= TY; ++step)
            {
                for (int y = 0; y < TY; ++y)
                    for (int x = 0; x < W; ++x)
                    {
                        hrow[y][x]    = 0;
                        zrow[y][x]    = 0;
                        expectH[y][x] = 0;
                        expectZ[y][x] = 0;
                    }

                //
                // A fresh InputFile for every pass, so each one starts
                // with an empty tile cache.
                //

                InputFile in (fn.c_str ());

                FrameBuffer fb;
                fb.insert (
                    "H",
                    Slice (
                        IMF::HALF,
                        (char*) &hrow[0][-DX],
                        sizeof (hrow[0][0]),
                        sizeof (hrow[0][0]) * W,
                        1,
                        1,
                        0.0,
                        false,
                        true));
                fb.insert (
                    "Z",
                    Slice (
                        IMF::HALF,
                        (char*) &zrow[0][-DX],
                        sizeof (zrow[0][0]),
                        sizeof (zrow[0][0]) * W,
                        1,
                        1,
                        fillZ,
                        false,
                        true));
                in.setFrameBuffer (fb);

                for (int i = 0; i < H; i += step)
                {
                    int lo = i;
                    int hi = std::min (i + step - 1, H - 1);

                    if (backwards)
                    {
                        lo = std::max (H - i - step, 0);
                        hi = H - 1 - i;
                    }

                    for (int y = lo; y <= hi; ++y)
                        for (int x = 0; x < W; ++x)
                        {
                            expectH[y % TY][x] = ph[y][x];
                            expectZ[y % TY][x] = fillZ;
                        }

                    if (backwards)
                        in.readPixels (DY + hi, DY + lo);
                    else
                        in.readPixels (DY + lo, DY + hi);

                    for (int y = 0; y < TY; ++y)
                        for (int x = 0; x < W; ++x)
                        {
                            assert (hrow[y][x] == expectH[y][x]);
                            assert (zrow[y][x] == expectZ[y][x]);
                        }
                }
            }
        }
    }

    remove (fn.c_str ());
}

int
testReread()
{
    //
    // Test reading a file then rereading with different channels, with
    // 32-bit channels.
    //

    int width  = 10;
    int height = 10;

    std::vector<float> R(width * height, 0.0f);
    std::vector<float> G(width * height, 0.0f);
    std::vector<float> B(width * height, 0.0f);

    const char* path = "reread32.exr";

    try {
        Header header(width, height);
        header.compression() = PIZ_COMPRESSION;
        header.channels().insert("R", Channel(FLOAT));
        header.channels().insert("G", Channel(FLOAT));
        header.channels().insert("B", Channel(FLOAT));

        OutputFile file(path, header);
        FrameBuffer fb;
        fb.insert("R", Slice(FLOAT, (char*)&R[0], sizeof(float), sizeof(float) * width));
        fb.insert("G", Slice(FLOAT, (char*)&G[0], sizeof(float), sizeof(float) * width));
        fb.insert("B", Slice(FLOAT, (char*)&B[0], sizeof(float), sizeof(float) * width));

        file.setFrameBuffer(fb);
        file.writePixels(height);

    } catch (const std::exception &e) {
        std::cerr << "Error writing image file " << path << ": " << e.what() << std::endl;
        return 1;
    }

    InputFile inFile(path);

    auto dw = inFile.header().dataWindow();

    int numChannels = 3;
    size_t chanBytes = sizeof(float);
    size_t pixBytes  = numChannels * chanBytes;
    size_t rowBytes  = width * pixBytes;
    std::vector<float> exr_RGB(width * height * numChannels);

    // Read RGB
    {
	FrameBuffer frameBuffer;
	frameBuffer.insert("R", Slice(FLOAT, (char*) exr_RGB.data() + (chanBytes * 0), pixBytes, rowBytes, 1, 1, 0.0) );
	frameBuffer.insert("G", Slice(FLOAT, (char*) exr_RGB.data() + (chanBytes * 1), pixBytes, rowBytes, 1, 1, 0.0) );
	frameBuffer.insert("B", Slice(FLOAT, (char*) exr_RGB.data() + (chanBytes * 2), pixBytes, rowBytes, 1, 1, 0.0) );
	inFile.setFrameBuffer(frameBuffer);
	inFile.readPixels(dw.min.y, dw.max.y);
    }

    // Reread just R
    {
	FrameBuffer frameBuffer;
	frameBuffer.insert("R", Slice(FLOAT, (char*) exr_RGB.data() + (chanBytes * 0), pixBytes, rowBytes, 1, 1, 0.0) );
	inFile.setFrameBuffer(frameBuffer);
	inFile.readPixels(dw.min.y, dw.max.y);
    }

    return 0;
}

void
testRereadFrameBuffer (const std::string& tempDir)
{
    //
    // readPixels(frameBuffer, y1, y2) must treat its frame buffer the
    // way setFrameBuffer() does: check the subsampling, fill the
    // frame buffer's own missing channels, and not reuse a decoder set
    // up for a different frame buffer.
    //

    cout << "Testing readPixels with a frame buffer argument" << endl;

    const int   W    = 16;
    const int   H    = 32;
    std::string path = tempDir + "imf_test_reread_fb.exr";

    {
        Header hdr (W, H);
        hdr.compression () = ZIP_COMPRESSION; // 16 lines per chunk
        hdr.channels ().insert ("R", Channel (HALF));
        hdr.channels ().insert ("G", Channel (HALF));
        hdr.channels ().insert ("B", Channel (HALF));

        vector<half> r (W * H, half (1.f));
        vector<half> g (W * H, half (2.f));
        vector<half> b (W * H, half (3.f));

        FrameBuffer fb;
        fb.insert ("R", Slice (HALF, (char*) r.data (), 2, 2 * W));
        fb.insert ("G", Slice (HALF, (char*) g.data (), 2, 2 * W));
        fb.insert ("B", Slice (HALF, (char*) b.data (), 2, 2 * W));

        OutputFile out (path.c_str (), hdr);
        out.setFrameBuffer (fb);
        out.writePixels (H);
    }

    InputFile in (path.c_str (), 0);

    //
    // Read a line as interleaved FLOAT RGB, then another line of the
    // same chunk as interleaved HALF RGB, into a buffer with a guard
    // region after the line.
    //

    vector<float> f (3 * W);
    FrameBuffer   ffb;
    ffb.insert ("R", Slice (FLOAT, (char*) &f[0], 12, 0));
    ffb.insert ("G", Slice (FLOAT, (char*) &f[1], 12, 0));
    ffb.insert ("B", Slice (FLOAT, (char*) &f[2], 12, 0));
    in.readPixels (ffb, 0, 0);
    assert (f[0] == 1.f && f[1] == 2.f && f[2] == 3.f);

    vector<half> h (6 * W, half (-1.f));
    FrameBuffer  hfb;
    hfb.insert ("R", Slice (HALF, (char*) &h[0], 6, 0));
    hfb.insert ("G", Slice (HALF, (char*) &h[1], 6, 0));
    hfb.insert ("B", Slice (HALF, (char*) &h[2], 6, 0));
    in.readPixels (hfb, 1, 1);
    for (int x = 0; x < W; ++x)
    {
        assert (h[3 * x + 0] == half (1.f));
        assert (h[3 * x + 1] == half (2.f));
        assert (h[3 * x + 2] == half (3.f));
    }
    for (int i = 3 * W; i < 6 * W; ++i)
        assert (h[i] == half (-1.f));

    //
    // Missing channels are filled in the frame buffer being read into,
    // not in the one last passed to setFrameBuffer().
    //

    vector<half> g1 (W), a1 (W), g2 (W), a2 (W);
    FrameBuffer  fb1;
    fb1.insert ("G", Slice (HALF, (char*) g1.data (), 2, 0));
    fb1.insert ("A", Slice (HALF, (char*) a1.data (), 2, 0, 1, 1, 0.25));
    in.setFrameBuffer (fb1);
    in.readPixels (0);
    assert (a1[0] == half (0.25f));

    a1.assign (W, half (0.f));
    FrameBuffer fb2;
    fb2.insert ("G", Slice (HALF, (char*) g2.data (), 2, 0));
    fb2.insert ("A", Slice (HALF, (char*) a2.data (), 2, 0, 1, 1, 0.5));
    in.readPixels (fb2, 20, 20);
    assert (g2[0] == half (2.f));
    for (int x = 0; x < W; ++x)
    {
        assert (a1[x] == half (0.f));
        assert (a2[x] == half (0.5f));
    }

    //
    // A 2x2 subsampled slice for a channel that isn't subsampled in
    // the file is rejected, as setFrameBuffer() rejects it.
    //

    vector<half> rs (W * H);
    FrameBuffer  sfb;
    sfb.insert ("R", Slice (HALF, (char*) rs.data (), 2, W, 2, 2));

    bool caught = false;
    try
    {
        in.readPixels (sfb, 0, H - 1);
    }
    catch (const IEX_NAMESPACE::ArgExc&)
    {
        caught = true;
    }
    assert (caught);

    remove (path.c_str ());
}

} // namespace

void
testScanLineApi (const std::string& tempDir)
{
    testReread();
    testRereadFrameBuffer (tempDir);

    try
    {
        cout << "Testing the scanline API for tiled files" << endl;

        const int W  = 48;
        const int H  = 81;
        const int DX = -17;
        const int DY = -29;

        Array2D<unsigned int> pi (H, W);
        Array2D<half>         ph (H, W);
        Array2D<float>        pf (H, W);
        fillPixels (pi, ph, pf, W, H);

        int maxThreads = ILMTHREAD_NAMESPACE::supportsThreads () ? 3 : 0;

        for (int n = 0; n <= maxThreads; ++n)
        {
            if (ILMTHREAD_NAMESPACE::supportsThreads ())
            {
                setGlobalThreadCount (n);
                cout << "\nnumber of threads: " << globalThreadCount () << endl;
            }

            for (int lorder = 0; lorder < NUM_LINEORDERS; ++lorder)
            {
                for (int rmode = 0; rmode < NUM_ROUNDINGMODES; ++rmode)
                {
                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        0,
                        0,
                        1,
                        1);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        DX,
                        DY,
                        1,
                        1);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        0,
                        0,
                        24,
                        26);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        DX,
                        DY,
                        24,
                        26);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        0,
                        0,
                        48,
                        81);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        DX,
                        DY,
                        48,
                        81);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        0,
                        0,
                        128,
                        96);

                    writeRead (
                        tempDir,
                        pi,
                        ph,
                        pf,
                        W,
                        H,
                        LineOrder (lorder),
                        ZIP_COMPRESSION,
                        LevelRoundingMode (rmode),
                        DX,
                        DY,
                        128,
                        96);
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

    testTileCoordsFill (tempDir);
}
