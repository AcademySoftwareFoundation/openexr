//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class Compressor
//
//-----------------------------------------------------------------------------

#include "ImfCheckedArithmetic.h"
#include "ImfNamespace.h"
#include "ImfCompressor.h"
#include "ImfB44Compressor.h"
#include "ImfDwaCompressor.h"
#include "ImfPizCompressor.h"
#include "ImfPxr24Compressor.h"
#include "ImfRleCompressor.h"
#include "ImfZipCompressor.h"
#include "ImfZstdCompressor.h"
#include "openexr_compression.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

using IMATH_NAMESPACE::Box2i;

Compressor::Compressor (const Header& hdr) : _header (hdr)
{}

Compressor::~Compressor ()
{}

Compressor::Format
Compressor::format () const
{
    return XDR;
}

int
Compressor::compressTile (
    const char* inPtr, int inSize, Box2i range, const char*& outPtr)
{
    return compress (inPtr, inSize, range.min.y, outPtr);
}

int
Compressor::uncompressTile (
    const char* inPtr, int inSize, Box2i range, const char*& outPtr)
{
    return uncompress (inPtr, inSize, range.min.y, outPtr);
}

Compressor*
newCompressor (Compression c, size_t maxScanLineSize, const Header& hdr)
{
    // clang-format off
    switch (c)
    {
        case RLE_COMPRESSION:

            return new RleCompressor (hdr, maxScanLineSize);

        case ZIPS_COMPRESSION:

            return new ZipCompressor (hdr, maxScanLineSize, 1);

        case ZIP_COMPRESSION:

            return new ZipCompressor (hdr, maxScanLineSize, 16);

        case PIZ_COMPRESSION:

            return new PizCompressor (hdr, maxScanLineSize, 32);

        case PXR24_COMPRESSION:

            return new Pxr24Compressor (hdr, maxScanLineSize, 16);

        case B44_COMPRESSION:

            return new B44Compressor (hdr, maxScanLineSize, 32, false);

        case B44A_COMPRESSION:

            return new B44Compressor (hdr, maxScanLineSize, 32, true);

        case DWAA_COMPRESSION:

            return new DwaCompressor (
                hdr,
                static_cast<int> (maxScanLineSize),
                32,
                DwaCompressor::STATIC_HUFFMAN);

        case DWAB_COMPRESSION:

            return new DwaCompressor (
                hdr,
                static_cast<int> (maxScanLineSize),
                256,
                DwaCompressor::STATIC_HUFFMAN);

        case ZSTD_COMPRESSION:

            return new ZstdCompressor (hdr);

        default: return 0;
    }
    // clang-format on
}

// for a given compression type, return the number of scanlines
// compressed into a single chunk
// TODO add to API and move to ImfCompressor.cpp
int
numLinesInBuffer (Compression comp)
{
    int numScanlines = getCompressionNumScanlines (comp);
    if (numScanlines < 1)
        throw IEX_NAMESPACE::ArgExc ("Unknown compression type");
    return numScanlines;
}

Compressor*
newTileCompressor (
    Compression c, size_t tileLineSize, size_t numTileLines, const Header& hdr)
{
    // clang-format off
    switch (c)
    {
        case RLE_COMPRESSION:

            return new RleCompressor (hdr, uiMult (tileLineSize, numTileLines));

        case ZIPS_COMPRESSION:
        case ZIP_COMPRESSION:

            return new ZipCompressor (hdr, tileLineSize, numTileLines);

        case PIZ_COMPRESSION:

            return new PizCompressor (hdr, tileLineSize, numTileLines);

        case PXR24_COMPRESSION:

            return new Pxr24Compressor (hdr, tileLineSize, numTileLines);

        case B44_COMPRESSION:

            return new B44Compressor (hdr, tileLineSize, numTileLines, false);

        case B44A_COMPRESSION:

            return new B44Compressor (hdr, tileLineSize, numTileLines, true);

        case DWAA_COMPRESSION:

            return new DwaCompressor (
                hdr,
                static_cast<int> (tileLineSize),
                static_cast<int> (numTileLines),
                DwaCompressor::DEFLATE);

        case DWAB_COMPRESSION:

            return new DwaCompressor (
                hdr,
                static_cast<int> (tileLineSize),
                static_cast<int> (numTileLines),
                DwaCompressor::STATIC_HUFFMAN);

        case ZSTD_COMPRESSION:

            return new ZstdCompressor (hdr, tileLineSize, numTileLines);

        default: return 0;
    }
    // clang-format on
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
