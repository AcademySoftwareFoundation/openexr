//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <ImathRandom.h>
#include <ImfRle.h>
#include <assert.h>
#include <iostream>
#include <string>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace std;

namespace
{

// Generate a random sequence of runs and single values
void
generateData (char* buffer, int bufferLen)
{
    char   value = 0;
    int    i     = 0;
    Rand48 rand48 (0);

    while (i < bufferLen)
    {

        if (rand48.nextf () < .5)
        {

            // Insert a single value
            buffer[i++] = value;
        }
        else
        {

            // Insert a run
            int runLen = (int) rand48.nextf (2.0, 1024.0);

            for (int j = 0; j < runLen; ++j)
            {
                buffer[i++] = value;
                if (i >= bufferLen) break;
            }
        }

        value = (int) (value + 1) & 0xff;
    }
}

// Compress, decompress, and compare with the original
void
testRoundTrip (int bufferLen)
{
    char*        src        = new char[bufferLen];
    signed char* compressed = new signed char[2 * bufferLen];
    char*        test       = new char[bufferLen];

    generateData (src, bufferLen);

    int compressedLen = rleCompress (bufferLen, src, compressed);

    assert (rleUncompress (compressedLen, bufferLen, compressed, test) > 0);

    for (int i = 0; i < bufferLen; ++i)
    {
        assert (src[i] == test[i]);
    }

    delete[] src;
    delete[] compressed;
    delete[] test;
}

} // namespace

void
testRle (const string&)
{
    cout << "RLE compression:" << endl;

    try
    {

        cout << "   Round tripping buffers " << endl;

        const int numIter = 1000;
        Rand48    rand48 (0);

        for (int iter = 0; iter < numIter; ++iter)
        {
            testRoundTrip ((int) rand48.nextf (100.0, 1000000.0));
        }
    }
    catch (const exception& e)
    {
        cout << "unexpected exception: " << e.what () << endl;
        assert (false);
    }
    catch (...)
    {
        cout << "unexpected exception" << endl;
        assert (false);
    }

    cout << "ok\n" << endl;
}
