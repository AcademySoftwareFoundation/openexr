//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <iostream>
#include <iomanip>

#include <ImfArray.h>
#include <ImfHeader.h>
#include <ImfPixelType.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfThreading.h>

#include <assert.h>
#include <stdio.h>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;

namespace {

enum FLIP_FLAGS
{
    FLIP_NONE = 0,
    FLIP_X    = 1,
    FLIP_Y    = 2
};

void
fillPixels (Array2D<float>& ph, int width, int height)
{
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            ph[y][x] = y * 1000 + x;
}

Slice filppedSlice (const Array2D<float>& ph, FLIP_FLAGS flip)
{
    cout << "flipX " << ((flip & FLIP_X) != 0)
         << ", flipY " << ((flip & FLIP_Y) != 0)
         << flush;

    char *base = (char *) &ph[0][0];
    ptrdiff_t strideX = sizeof (ph[0][0]);
    ptrdiff_t strideY = sizeof (ph[0][0]) * ph.width ();
    if (flip & FLIP_X)
    {
        base += strideX * (ph.width () - 1);
        strideX = -strideX;
    }
    if (flip & FLIP_Y)
    {
        base += strideY * (ph.height () - 1);
        strideY = -strideY;
    }
    return Slice (FLOAT, base, strideX, strideY);
}

void
writeRead (
    const Array2D<float>& ph,
    const char           fileName[],
    FLIP_FLAGS           writeFlip,
    FLIP_FLAGS           readFlip)
{
    int width = ph.width ();
    int height = ph.height ();
    {
        Header hdr (width, height);
        hdr.channels ().insert ("F", Channel (FLOAT));

        FrameBuffer fb;
        fb.insert ("F", filppedSlice (ph, writeFlip));

        cout << " writing" << flush;

        remove (fileName);
        OutputFile out (fileName, hdr);
        out.setFrameBuffer (fb);
        out.writePixels (height);
    }
    {
        cout << " ";
        FrameBuffer fb;
        Array2D<float> ph2 (height, width);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                ph2[y][x] = -1.0f;
        fb.insert ("F", filppedSlice (ph2, readFlip));

        cout << " reading" << flush;

        InputFile in (fileName);
        in.setFrameBuffer (fb);
        in.readPixels (0, height - 1);

        cout << " comparing" << flush;

        FLIP_FLAGS flip = (FLIP_FLAGS) (writeFlip ^ readFlip);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int x2 = x;
                int y2 = y;

                if (flip & FLIP_X)
                    x2 = width - 1 - x;

                if (flip & FLIP_Y)
                    y2 = height - 1 - y;

                assert (ph[y][x] == ph2[y2][x2]);
            }   
        }
    }
}

} // namespace

void
testNegativeStride (const string& tempDir)
{
    try
    {
        cout << "Testing negative slice stride" << endl;

        const int W = 117;
        const int H = 97;

        Array2D<float> ph (H, W);
        fillPixels (ph, W, H);

        string fileName = tempDir + "imf_test_negative_stride.exr";
        for (int readFlip = 0; readFlip < 4; ++readFlip)
            for (int writeFlip = 0; writeFlip < 4; ++writeFlip)
            {
                writeRead (ph, fileName.c_str (),
                           (FLIP_FLAGS) writeFlip, (FLIP_FLAGS) readFlip);
                cout << endl;
            }

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
