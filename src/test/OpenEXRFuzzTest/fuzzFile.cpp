//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include <fuzzFile.h>

#include <Iex.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <half.h>

#include "../OpenEXRTest/TestUtilFStream.h"
#include <fstream>
#include <iostream>

// Handle the case when the custom namespace is not exposed
#include <OpenEXRConfig.h>
using namespace OPENEXR_IMF_INTERNAL_NAMESPACE;
using namespace std;
using namespace IMATH_NAMESPACE;

namespace
{

uint64_t
lengthOfFile (const char fileName[])
{
    ifstream ifs;
    testutil::OpenStreamWithUTF8Name (
        ifs, fileName, ios::in | ios_base::binary);

    if (!ifs) return 0;

    ifs.seekg (0, ios_base::end);
    return ifs.tellg ();
}

void
fuzzFile (
    const char goodFile[],
    const char brokenFile[],
    uint64_t   offset,
    uint64_t   windowSize,
    Rand48&    random,
    double     fuzzAmount)
{
    //
    // Read the input file.
    //

    ifstream ifs;
    testutil::OpenStreamWithUTF8Name (
        ifs, goodFile, ios::in | ios_base::binary);

    if (!ifs) THROW_ERRNO ("Cannot open file " << goodFile << " (%T).");

    ifs.seekg (0, ios_base::end);
    uint64_t fileLength = ifs.tellg ();
    ifs.seekg (0, ios_base::beg);

    Array<char> data (fileLength);
    ifs.read (data, fileLength);

    if (!ifs) THROW_ERRNO ("Cannot read file " << goodFile << " (%T)." << endl);

    //
    // Damage the contents of the file by overwriting some of the bytes
    // in a window of size windowSize, starting at the specified offset.
    //

    for (uint64_t i = offset; i < offset + windowSize; ++i)
    {
        if (random.nextf () < fuzzAmount) data[i] = char (random.nexti ());
    }

    //
    // Save the damaged file contents in the output file.
    //

    ofstream ofs;
    testutil::OpenStreamWithUTF8Name (
        ofs, brokenFile, ios::out | ios_base::binary);

    if (!ofs)
        THROW_ERRNO ("Cannot open file " << brokenFile << " (%T)." << endl);

    ofs.write (data, fileLength);

    if (!ofs)
        THROW_ERRNO ("Cannot write file " << brokenFile << " (%T)." << endl);
}

} // namespace

void
fuzzFile (
    const char goodFile[],
    const char brokenFile[],
    void (*readFile) (const char[]),
    int     nSlidingWindow,
    int     nFixedWindow,
    Rand48& random)
{
    //
    // We want to test how resilient the OpenEXR library is with respect
    // to malformed OpenEXR input files.  In order to do this we damage
    // a good input file by overwriting parts of it with random data.
    // We then call function readFile() to try and read the damaged file.
    // Provided the OpenEXR library works as advertised, a try/catch(...)
    // block in readFile() should be able to handle all errors that could
    // possibly result from reading a broken OpenEXR file.  We repeat
    // this damage/read cycle many times, overwriting different parts
    // of the file:
    //
    // First we slide a window along the file.  The size of the window
    // is fileSize*2/nSlidingWindow bytes.  In each damage/read cycle
    // we overwrite up to 10% of the bytes the window, try to read the
    // file, and advance the window by fileSize/nSlidingWindow bytes.
    //
    // Next we overwrite up to 10% of the file's first 2048 bytes and
    // try to read the file.  We repeat this nFixedWindow times.
    //

    {
        uint64_t fileSize         = lengthOfFile (goodFile);
        uint64_t windowSize       = fileSize * 2 / nSlidingWindow;
        uint64_t lastWindowOffset = fileSize - windowSize;

        cout << "sliding " << windowSize << "-byte window" << endl;

        for (int i = 0; i < nSlidingWindow; ++i)
        {
            if (i % 100 == 0) cout << i << "\r" << flush;

            uint64_t offset     = lastWindowOffset * i / (nSlidingWindow - 1);
            double   fuzzAmount = random.nextf (0.0, 0.1);

            fuzzFile (
                goodFile, brokenFile, offset, windowSize, random, fuzzAmount);

            readFile (brokenFile);
        }

        cout << nSlidingWindow << endl;
    }

    {
        uint64_t windowSize = 2048;

        cout << windowSize << "-byte window at start of file" << endl;

        for (int i = 0; i < nFixedWindow; ++i)
        {
            if (i % 100 == 0) cout << i << "\r" << flush;

            double fuzzAmount = random.nextf (0.0, 0.1);

            fuzzFile (goodFile, brokenFile, 0, windowSize, random, fuzzAmount);

            readFile (brokenFile);
        }

        cout << nFixedWindow << endl;
    }
}
