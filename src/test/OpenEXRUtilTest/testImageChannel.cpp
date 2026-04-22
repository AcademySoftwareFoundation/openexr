//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "testImageChannel.h"

#include "ImfFlatImage.h"

#include <Iex.h>
#include <ImathBox.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;
using namespace IEX_NAMESPACE;
using namespace std;

namespace
{

//
// Sampling must be >= 1. ImageChannel::resize() rejects non-positive
// sampling before any modulo or division (so zero cannot invoke undefined
// behavior). Other invalid cases are still caught later in resize().
//

void
expectArgExcOnInsert (const char* label, int xSampling, int ySampling)
{
    FlatImage img (Box2i (V2i (0, 0), V2i (3, 3)));

    try
    {
        img.insertChannel ("bad", HALF, xSampling, ySampling, false);
        cerr << "ERROR -- " << label << ": expected ArgExc, insert succeeded"
             << endl;
        assert (false);
    }
    catch (const ArgExc& e)
    {
        // Thrown at the start of ImageChannel::resize() so % and / never use
        // a zero sampling rate (undefined behavior).
        assert (strstr (e.what (), "at least 1") != nullptr);
    }
}

} // namespace

void
testImageChannel (const string& tempDir)
{
    (void) tempDir;

    try
    {
        cout << "Testing ImageChannel sampling constraints" << endl;

        expectArgExcOnInsert ("negative xSampling", -1, 1);
        expectArgExcOnInsert ("negative ySampling", 1, -1);
        expectArgExcOnInsert ("negative x and y sampling", -1, -1);

        expectArgExcOnInsert ("zero xSampling", 0, 1);
        expectArgExcOnInsert ("zero ySampling", 1, 0);
        expectArgExcOnInsert ("zero x and y sampling", 0, 0);

        cout << "ok\n" << endl;
    }
    catch (const std::exception& e)
    {
        cerr << "ERROR -- caught exception: " << e.what () << endl;
        assert (false);
    }
}
