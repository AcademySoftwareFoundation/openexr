//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_ZSTD_COMPRESSOR_H
#define INCLUDED_IMF_ZSTD_COMPRESSOR_H

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class ZstdCompressor : public Compressor
{
public:
    ZstdCompressor (const Header& hdr, size_t maxScanLineSize, int scanLines);
    virtual ~ZstdCompressor ();

    ZstdCompressor (const ZstdCompressor& other)            = delete;
    ZstdCompressor& operator= (const ZstdCompressor& other) = delete;
    ZstdCompressor (ZstdCompressor&& other)                 = delete;
    ZstdCompressor& operator= (ZstdCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif