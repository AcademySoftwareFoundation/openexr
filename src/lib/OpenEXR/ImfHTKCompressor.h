//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_HTK_COMPRESSOR_H
#define INCLUDED_IMF_HTK_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class HTKCompressor -- uses High-Throughput JPEG 2000.
//
//-----------------------------------------------------------------------------

#include <vector>

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class HTKCompressor : public Compressor
{
public:
    HTKCompressor (const Header& hdr, size_t maxScanLineSize, int numScanLines);

    virtual ~HTKCompressor ();

    HTKCompressor (const HTKCompressor& other) = delete;
    HTKCompressor& operator= (const HTKCompressor& other) = delete;
    HTKCompressor (HTKCompressor&& other)                 = delete;
    HTKCompressor& operator= (HTKCompressor&& other) = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
