//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_RLE_COMPRESSOR_H
#define INCLUDED_IMF_RLE_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class RleCompressor -- performs run-length encoding
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class RleCompressor : public Compressor
{
public:
    RleCompressor (const Header& hdr, size_t maxScanLineSize);
    virtual ~RleCompressor ();

    RleCompressor (const RleCompressor& other)            = delete;
    RleCompressor& operator= (const RleCompressor& other) = delete;
    RleCompressor (RleCompressor&& other)                 = delete;
    RleCompressor& operator= (RleCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
