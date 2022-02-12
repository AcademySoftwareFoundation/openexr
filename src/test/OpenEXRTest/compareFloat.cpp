//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifdef NDEBUG
#    undef NDEBUG
#endif

#include "compareFloat.h"

using namespace OPENEXR_IMF_NAMESPACE;

bool
equivalent (float f1, float f2, Compression comp)
{
    //
    // Test if a float, f1, is "equivalent" to another float, f2,
    // which results from storing f1 in an image file, and reading
    // it back:
    // If the file was compressed with PXR24_COMPRESSION, then f1
    // and f2 must the same as f1 rounded to 24 bits (see class
    // ImfPxr24Compressor); otherwise f1 and f2 must be the same.
    //

    union
    {
        float        f;
        unsigned int i;
    } u1, u2;

    u1.f = f1;
    u2.f = f2;

    if (comp == PXR24_COMPRESSION)
        return (u2.i >> 8) - (u1.i >> 8) < 2;
    else
        return u2.i == u1.i;
}
