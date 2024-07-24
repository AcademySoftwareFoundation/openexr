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
        const Header& hdr, size_t tileLineSize = 0, size_t numTileLines = 0);

private:
    using data_ptr = std::unique_ptr<char, decltype (&free)>;
    std::vector<data_ptr> _outBuffer;
    int                   numScanLines () const override; // max
    int                   compress (
                          const char* inPtr, int inSize, int minY, const char*& outPtr) override;
    int uncompress (
        const char* inPtr, int inSize, int minY, const char*& outPtr) override;
    int sampleCountTableSize ();

    int  m_tileLineSize;
    int  m_numTileLines;
    int  m_tileArea;
    bool m_isTiled;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT