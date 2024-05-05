//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#ifndef INCLUDED_IMF_DWA_COMRESSOR_H
#define INCLUDED_IMF_DWA_COMRESSOR_H

//------------------------------------------------------------------------------
//
// class DwaCompressor -- Store lossy RGB data by quantizing DCT components.
//
//------------------------------------------------------------------------------

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class DwaCompressor : public Compressor
{
public:
    enum AcCompression
    {
        STATIC_HUFFMAN,
        DEFLATE,
    };

    DwaCompressor (
        const Header& hdr,
        size_t        maxScanLineSize,
        int           numScanLines, // ideally is a multiple of 8
        AcCompression acCompression);

    virtual ~DwaCompressor ();

    DwaCompressor (const DwaCompressor& other)            = delete;
    DwaCompressor& operator= (const DwaCompressor& other) = delete;
    DwaCompressor (DwaCompressor&& other)                 = delete;
    DwaCompressor& operator= (DwaCompressor&& other)      = delete;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
