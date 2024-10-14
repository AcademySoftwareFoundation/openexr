//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "compareB44.h"

#include "compareFloat.h"
#include <ImathRandom.h>
#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfCompressor.h>
#include <ImfFrameBuffer.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfTiledOutputFile.h>
#include <half.h>

#include <algorithm>
#include <assert.h>
#include <limits>
#include <stdio.h>

namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace IMF;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

struct pixelArray
{
    Array2D<half> h;
    Array2D<half> rgba[4];
    pixelArray (int height, int width) : h (height, width)
    {
        for (int c = 0; c < 4; ++c)
        {
            rgba[c].resizeErase (height, width);
        }
    }
};

void
fillPixels1 (pixelArray& array, int width, int height)
{
    cout << "only zeroes" << endl;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            array.h[y][x] = 0;
            for (int c = 0; c < 4; ++c)
            {
                array.rgba[c][y][x] = 0;
            }
        }
}

void
fillPixels2 (pixelArray& array, int width, int height)
{
    cout << "pattern 1" << endl;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            array.h[y][x] = (x + y) & 1;
            for (int c = 0; c < 4; ++c)
            {
                array.rgba[c][y][x] = array.h[y][x];
            }
        }
}

void
fillPixels3 (pixelArray& array, int width, int height)
{
    cout << "pattern 2" << endl;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {

            array.h[y][x] = sin (double (x)) + sin (y * 0.5);
            for (int c = 0; c < 4; ++c)
            {
                array.rgba[c][y][x] = sin (double (x + c)) + sin (y * 0.5);
            }
        }
}

void
fillPixels4 (pixelArray& array, int width, int height)
{
    cout << "random bits" << endl;

    //
    // Use of a union to extract the bit pattern from a float, as is
    // done below, works only if int and float have the same size.
    //

    assert (sizeof (int) == sizeof (float));

    Rand48 rand;

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {

            array.h[y][x].setBits (rand.nexti ());
            for (int c = 0; c < 4; ++c)
            {
                array.rgba[c][y][x].setBits (rand.nexti ());
            }

            union
            {
                int   i;
                float f;
            } u;
            u.i = rand.nexti ();
        }
}

void
writeRead (
    pixelArray& ref_array,
    const char  fileName[],
    int         width,
    int         height,
    Compression comp)
{

    //
    // Write the pixel data in pi1, ph1 and ph2 to an
    // image file using the specified compression type
    // and subsampling rates.  Read the pixel data back
    // from the file and verify that the data did not
    // change.
    //

    cout << "compression " << comp << flush;

    Header hdr (
        (Box2i (
            V2i (0, 0), // display window
            V2i (width - 1, height - 1))),
        (Box2i (
            V2i (0, 0), // data window
            V2i (width - 1, height - 1))));

    hdr.compression ()         = comp;
    hdr.zipCompressionLevel () = 4;

    static const char* channels[] = {"R", "G", "B", "A", "H"};

    for (int c = 0; c < 5; ++c)
    {
        hdr.channels ().insert (
            channels[c], // name
            Channel (
                IMF::HALF, // type
                1,         // xSampling
                1)         // ySampling
        );
    }

    {
        FrameBuffer fb;

        fb.insert (
            "H", // name
            Slice (
                IMF::HALF,                       // type
                (char*) &ref_array.h[0][0],         // base
                sizeof (ref_array.h[0][0]),         // xStride
                sizeof (ref_array.h[0][0]) * width, // yStride
                1,                               // xSampling
                1)                               // ySampling
        );

        for (int c = 0; c < 4; c++)
        {
            fb.insert (
                channels[c], // name
                Slice (
                    IMF::HALF,                             // type
                    (char*) &ref_array.rgba[c][0][0],         // base
                    sizeof (ref_array.rgba[c][0][0]),         // xStride
                    sizeof (ref_array.rgba[c][0][0]) * width, // yStride
                    1,                                     // xSampling
                    1)                                     // ySampling
            );
        }

        cout << " writing" << flush;

        remove (fileName);

        OutputFile out (fileName, hdr);
        out.setFrameBuffer (fb);
        out.writePixels (height);
    }

    {
        cout << " reading" << flush;
        InputFile in (fileName);

        const Box2i& dw = hdr.dataWindow ();
        int          w  = dw.max.x - dw.min.x + 1;
        int          h  = dw.max.y - dw.min.y + 1;
        int          dx = dw.min.x;
        int          dy = dw.min.y;

        pixelArray  decoded_array (h, w);
        FrameBuffer fb;

        {
            int xs = in.header ().channels ()["H"].xSampling;
            int ys = in.header ().channels ()["H"].ySampling;

            fb.insert (
                "H", // name
                Slice (
                    IMF::HALF,                   // type
                    (char*) &decoded_array.h[-dy][-dx], // base
                    sizeof (decoded_array.h[0][0]),     // xStride
                    sizeof (decoded_array.h[0][0]) * w, // yStride
                    1,                           // xSampling
                    1)                           // ySampling
            );
        }

        for (int c = 0; c < 4; ++c)
        {
            int xs = in.header ().channels ()[channels[c]].xSampling;
            int ys = in.header ().channels ()[channels[c]].ySampling;

            fb.insert (
                channels[c], // name
                Slice (
                    IMF::HALF,                           // type
                    (char*) &decoded_array.rgba[c][-dy][-dx],   // base
                    sizeof (decoded_array.rgba[c][0][0]),       // xStride
                    sizeof (decoded_array.rgba[c][0][0]) * (w), // yStride
                    1,                                   // xSampling
                    1)                                   // ySampling
            );
        }

        in.setFrameBuffer (fb);
        in.readPixels (dw.min.y, dw.max.y);

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

                if (!isLossyCompression (comp))
                {
                    assert (ref_array.h[y][x].bits () == decoded_array.h[y][x].bits ());
                    for (int c = 0; c < 4; ++c)
                    {
                        assert (
                            ref_array.rgba[c][y][x].bits () ==
                            decoded_array.rgba[c][y][x].bits ());
                    }
                }
            }
        }
    }

    remove (fileName);
    cout << endl;
}

void
writeRead (
    const std::string& tempDir, pixelArray& array, int w, int h, int dx, int dy)
{
    std::string filename = tempDir + "imf_test_comp.exr";

    writeRead (array, filename.c_str (), w, h, HT_COMPRESSION);
    writeRead (array, filename.c_str (), w, h, HT256_COMPRESSION);
    writeRead (array, filename.c_str (), w, h, HTK_COMPRESSION);
    writeRead (array, filename.c_str (), w, h, HTK256_COMPRESSION);
}

} // namespace

void
testHTCompression (const std::string& tempDir)
{
    try
    {
        cout << "Testing pixel data types, "
                "subsampling and "
                "compression schemes"
             << endl;

        const int W  = 1371;
        const int H  = 159;
        const int DX = 17;
        const int DY = 29;

        pixelArray array (H, W);

        //
        // If the following assertion fails, new pixel types have
        // been added to the Imf library; testing code for the new
        // pixel types should be added to this file.
        //

        assert (NUM_PIXELTYPES == 3);

        fillPixels1 (array, W, H);
        writeRead (tempDir, array, W, H, DX, DY);

        fillPixels2 (array, W, H);
        writeRead (tempDir, array, W, H, DX, DY);

        fillPixels3 (array, W, H);
        writeRead (tempDir, array, W, H, DX, DY);

        fillPixels4 (array, W, H);
        writeRead (tempDir, array, W, H, DX, DY);

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
