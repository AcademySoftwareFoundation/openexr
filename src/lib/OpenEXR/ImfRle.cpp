//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#include "ImfRle.h"
#include "ImfNamespace.h"
#include "openexr_compression.h"
#include <string.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER


int
rleCompress (int inLength, const char in[], signed char out[])
{
    // assume we have 3/2 like in RleCompressor
    return static_cast<int> (
        exr_rle_compress_buffer (
            (size_t) inLength,
            in,
            out,
            (((size_t) inLength) * 3) / 2));
}

//
// Uncompress an array of bytes compressed with rleCompress().
// Returns the length of the oncompressed data, or 0 if the
// length of the uncompressed data would be more than maxLength.
//

int
rleUncompress (int inLength, int maxLength, const signed char in[], char out[])
{
    return static_cast<int> (
        exr_rle_uncompress_buffer (
            (size_t) inLength,
            (size_t) maxLength,
            in,
            out));
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
