//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Weta Digital, Ltd and Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "random.h"
#include "testCompositeDeepScanLine.h"

#include <ImfChannelList.h>
#include <ImfDeepFrameBuffer.h>
#include <ImfDeepScanLineInputFile.h>
#include <ImfDeepScanLineOutputFile.h>
#include <ImfHeader.h>
#include <ImfNamespace.h>
#include <ImfPartType.h>

#include <vector>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "tmpDir.h"

namespace
{

using std::cout;
using std::endl;
using std::flush;
using std::vector;

using IMATH_NAMESPACE::Box2i;
using OPENEXR_IMF_NAMESPACE::Channel;
using OPENEXR_IMF_NAMESPACE::DeepFrameBuffer;
using OPENEXR_IMF_NAMESPACE::DEEPSCANLINE;
using OPENEXR_IMF_NAMESPACE::DeepScanLineInputFile;
using OPENEXR_IMF_NAMESPACE::DeepScanLineOutputFile;
using OPENEXR_IMF_NAMESPACE::DeepSlice;
using OPENEXR_IMF_NAMESPACE::FLOAT;
using OPENEXR_IMF_NAMESPACE::Header;
using OPENEXR_IMF_NAMESPACE::Slice;
using OPENEXR_IMF_NAMESPACE::UINT;
using OPENEXR_IMF_NAMESPACE::ZIPS_COMPRESSION;

namespace IMF = OPENEXR_IMF_NAMESPACE;

static void
make_file (const char* filename)
{

    int width  = 4;
    int height = 48;

    //
    // create a deep output file of widthxheight, where each pixel has 'y' samples,
    // each with value 'x'
    //

    Header header (width, height);
    header.channels ().insert ("Z", Channel (IMF::FLOAT));
    header.compression () = ZIPS_COMPRESSION;
    header.setType (DEEPSCANLINE);

    remove (filename);
    DeepScanLineOutputFile file (filename, header);

    unsigned int sample_count;
    float        sample;
    float*       sample_ptr = &sample;

    DeepFrameBuffer fb;

    fb.insertSampleCountSlice (Slice (IMF::UINT, (char*) &sample_count));
    fb.insert ("Z", DeepSlice (IMF::FLOAT, (char*) &sample_ptr));

    file.setFrameBuffer (fb);

    for (int y = 0; y < height; y++)
    {
        //
        // ensure each scanline contains a different number of samples,
        // with different values. We don't care that each sample has the same
        // value, or that each pixel on the scanline is identical
        //
        sample_count = y;
        sample       = y + 100.0;

        file.writePixels (1);
    }
}

static void
read_file (const char* filename)
{
    DeepScanLineInputFile file (filename);

    Box2i       datawin  = file.header ().dataWindow ();
    int         width    = datawin.size ().x + 1;
    int         height   = datawin.size ().y + 1;
    int         x_offset = datawin.min.x;
    int         y_offset = datawin.min.y;
    const char* channel  = file.header ().channels ().begin ().name ();

    vector<unsigned int> samplecounts (width);
    vector<float*>       sample_pointers (width);
    vector<float>        samples;

    DeepFrameBuffer fb;

    fb.insertSampleCountSlice (Slice (
        IMF::UINT,
        (char*) (&samplecounts[0] - x_offset),
        sizeof (unsigned int)));

    fb.insert (
        channel,
        DeepSlice (
            IMF::FLOAT,
            (char*) (&sample_pointers[0] - x_offset),
            sizeof (float*),
            0,
            sizeof (float)));

    file.setFrameBuffer (fb);

    for (int count = 0; count < 4000; count++)
    {
        int row = random_int (height) + y_offset;

        //
        // read row y (at random)
        //

        file.readPixelSampleCounts (row, row);
        //
        // check that's correct, and also resize samples array
        //

        int total_samples = 0;
        for (int i = 0; i < width; i++)
        {

            if (samplecounts[i] != static_cast<unsigned int> (row))
            {
                cout << i << ", " << row << " error, sample counts should be "
                     << row << ", is " << samplecounts[i] << endl
                     << flush;
            }

            assert (samplecounts[i] == static_cast<unsigned int> (row));

            total_samples += samplecounts[i];
        }

        samples.resize (total_samples);
        //
        // set pointers to point to the correct place
        //
        int total = 0;
        for (int i = 0; i < width && total < total_samples; i++)
        {
            sample_pointers[i] = &samples[total];
            total += samplecounts[i];
        }

        //
        // read channel
        //

        file.readPixels (row, row);

        //
        // check
        //

        for (int i = 0; i < total_samples; i++)
        {
            if (samples[i] != row + 100.f)
            {
                cout << " sample " << i << " on row " << row
                     << " error, should be " << 100.f + row << " got "
                     << samples[i] << endl;
                cout << flush;
            }
            assert (samples[i] == row + 100.f);
        }
    }
}
} // namespace

void
testDeepScanLineMultipleRead (const std::string& tempDir)
{

    cout << "\n\nTesting random re-reads from deep scanline file:\n" << endl;

    std::string source_filename = tempDir + "imf_test_multiple_read";
    random_reseed (1);

    make_file (source_filename.c_str ());
    read_file (source_filename.c_str ());
    remove (source_filename.c_str ());

    cout << " ok\n" << endl;
}
