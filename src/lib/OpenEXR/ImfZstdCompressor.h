//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#pragma once

#include <memory>
#include "ImfNamespace.h"
#include "ImfCompressor.h"
#include "ImfHeader.h"
#include "blosc2.h"
#include "vector"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class ZstdCompressor : public Compressor
{
public:
    explicit ZstdCompressor (
        const Header& hdr,      // image header
        size_t maxBytesPerLine, // max number of bytes per uncompressed line
        size_t numTileLines = 0 // tiled image only
    );

private:
    using data_ptr = std::unique_ptr<char, decltype (&free)>;
    std::vector<data_ptr> _outBuffer;

    int numScanLines () const override; // max
    int compress (
        const char*  inPtr,
        int          inSize,
        const int*   inSampleCountPerLine,
        int          minY,
        const char*& outPtr) override;
    int uncompress (
        const char*  inPtr,
        int          inSize,
        const int*   sampleCountPerLine,
        int          minY,
        const char*& outPtr) override;

    int m_maxBytesPerLine; // max number of bytes per uncompressed tile line.
    int m_numTileLines;    // number of lines in a tile. 0 if scanline !
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT