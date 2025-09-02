//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_HT_COMPRESSOR_H
#define INCLUDED_IMF_HT_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class HTCompressor -- uses High-Throughput JPEG 2000.
//
//-----------------------------------------------------------------------------

#include <vector>

#include "ImfNamespace.h"

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class HTCompressor : public Compressor
{
public:
    HTCompressor (const Header& hdr, size_t maxScanLineSize, int numScanLines);
    virtual ~HTCompressor ();

    HTCompressor (const HTCompressor& other)            = delete;
    HTCompressor& operator= (const HTCompressor& other) = delete;
    HTCompressor (HTCompressor&& other)                 = delete;
    HTCompressor& operator= (HTCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
