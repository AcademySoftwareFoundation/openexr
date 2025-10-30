//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_GDEFLATE_COMPRESSOR_H
#define INCLUDED_IMF_GDEFLATE_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//  class GdeflateCompressor -- performs gdeflate compression
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class GdeflateCompressor : public Compressor
{
public:
    GdeflateCompressor (
        const Header& hdr, size_t maxScanLineSize, int numScanLines);

    virtual ~GdeflateCompressor ();

    GdeflateCompressor (const GdeflateCompressor& other)            = delete;
    GdeflateCompressor& operator= (const GdeflateCompressor& other) = delete;
    GdeflateCompressor (GdeflateCompressor&& other)                 = delete;
    GdeflateCompressor& operator= (GdeflateCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif



