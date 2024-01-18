//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

// Mandatory section to register this compression method in OpenEXR
// ----------------------------------------------------------------
// ZIPS_COMPRESSION = 2
// ZIPS_COMPRESSION name zips
// ZIPS_COMPRESSION desc zlib compression, one scan line at a time.
// ZIPS_COMPRESSION scanlines 1
// ZIPS_COMPRESSION lossy false
// ZIPS_COMPRESSION deep true
// ZIPS_COMPRESSION newscan ZipCompressor (hdr, maxScanLineSize, 1)
// ZIPS_COMPRESSION newtile ZipCompressor (hdr, tileLineSize, numTileLines)
// ----------------------------------------------------------------
// ZIP_COMPRESSION = 3
// ZIP_COMPRESSION name zip
// ZIP_COMPRESSION desc zlib compression, in blocks of 16 scan lines.
// ZIP_COMPRESSION scanlines 16
// ZIP_COMPRESSION lossy false
// ZIP_COMPRESSION deep false
// ZIP_COMPRESSION newscan ZipCompressor (hdr, maxScanLineSize, 16)
// ZIP_COMPRESSION newtile ZipCompressor (hdr, tileLineSize, numTileLines)


#ifndef INCLUDED_IMF_ZIP_COMPRESSOR_H
#define INCLUDED_IMF_ZIP_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class ZipCompressor -- performs zlib-style compression
//
//-----------------------------------------------------------------------------

#include "ImfNamespace.h"

#include "ImfCompressor.h"

#include "ImfZip.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class ZipCompressor : public Compressor
{
public:
    ZipCompressor (
        const Header& hdr, size_t maxScanLineSize, size_t numScanLines);

    virtual ~ZipCompressor ();

    virtual int numScanLines () const;

    virtual int
    compress (const char* inPtr, int inSize, int minY, const char*& outPtr);

    virtual int
    uncompress (const char* inPtr, int inSize, int minY, const char*& outPtr);

private:
    int   _maxScanLineSize;
    int   _numScanLines;
    char* _outBuffer;
    Zip   _zip;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
