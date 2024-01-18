//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// Mandatory section to register this compression method in OpenEXR
// ----------------------------------------------------------------
// B44_COMPRESSION = 6
// B44_COMPRESSION name b44
// B44_COMPRESSION desc lossy 4-by-4 pixel block compression, fixed compression rate.
// B44_COMPRESSION scanlines 32
// B44_COMPRESSION lossy true
// B44_COMPRESSION deep false
// B44_COMPRESSION newscan B44Compressor (hdr, maxScanLineSize, 32, false)
// B44_COMPRESSION newtile B44Compressor (hdr, tileLineSize, numTileLines, false)
// ----------------------------------------------------------------
// B44A_COMPRESSION = 7
// B44A_COMPRESSION name b44a
// B44A_COMPRESSION desc lossy 4-by-4 pixel block compression, flat fields are compressed more.
// B44A_COMPRESSION scanlines 32
// B44A_COMPRESSION lossy true
// B44A_COMPRESSION deep false
// B44A_COMPRESSION newscan B44Compressor (hdr, maxScanLineSize, 32, true)
// B44A_COMPRESSION newtile B44Compressor (hdr, tileLineSize, numTileLines, true)

#ifndef INCLUDED_IMF_B44_COMPRESSOR_H
#define INCLUDED_IMF_B44_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class B44Compressor -- lossy compression of 4x4 pixel blocks
//
//-----------------------------------------------------------------------------

#include "ImfForward.h"

#include "ImfCompressor.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class B44Compressor : public Compressor
{
public:
    B44Compressor (
        const Header& hdr,
        size_t        maxScanLineSize,
        size_t        numScanLines,
        bool          optFlatFields);

    virtual ~B44Compressor ();

    B44Compressor (const B44Compressor& other) = delete;
    B44Compressor& operator= (const B44Compressor& other) = delete;
    B44Compressor (B44Compressor&& other)                 = delete;
    B44Compressor& operator= (B44Compressor&& other) = delete;

    virtual int numScanLines () const;

    virtual Format format () const;

    virtual int
    compress (const char* inPtr, int inSize, int minY, const char*& outPtr);

    virtual int compressTile (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    virtual int
    uncompress (const char* inPtr, int inSize, int minY, const char*& outPtr);

    virtual int uncompressTile (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

private:
    struct ChannelData;

    int compress (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    int uncompress (
        const char*            inPtr,
        int                    inSize,
        IMATH_NAMESPACE::Box2i range,
        const char*&           outPtr);

    bool               _optFlatFields;
    Format             _format;
    int                _numScanLines;
    unsigned short*    _tmpBuffer;
    char*              _outBuffer;
    int                _numChans;
    const ChannelList& _channels;
    ChannelData*       _channelData;
    int                _minX;
    int                _maxX;
    int                _maxY;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
