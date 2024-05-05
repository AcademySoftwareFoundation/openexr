//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_PIZ_COMPRESSOR_H
#define INCLUDED_IMF_PIZ_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class PizCompressor -- uses Wavelet and Huffman encoding.
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class PizCompressor : public Compressor
{
public:
    PizCompressor (
        const Header& hdr, size_t maxScanLineSize, int numScanLines);

    virtual ~PizCompressor ();

    PizCompressor (const PizCompressor& other)            = delete;
    PizCompressor& operator= (const PizCompressor& other) = delete;
    PizCompressor (PizCompressor&& other)                 = delete;
    PizCompressor& operator= (PizCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
