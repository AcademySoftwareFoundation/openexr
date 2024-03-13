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
    explicit ZstdCompressor (const Header& hdr);

private:
    using raw_ptr = std::unique_ptr<char, decltype (&free)>;
    std::vector<raw_ptr> _outBuffer;
    int                  numScanLines () const override; // max
    int                  compress (
                         const char* inPtr, int inSize, int minY, const char*& outPtr) override;
    int uncompress (
        const char* inPtr, int inSize, int minY, const char*& outPtr) override;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT