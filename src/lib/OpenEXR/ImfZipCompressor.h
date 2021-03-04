//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//


#ifndef INCLUDED_IMF_ZIP_COMPRESSOR_H
#define INCLUDED_IMF_ZIP_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class ZipCompressor -- performs zlib-style compression
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"
#include "ImfZip.h"
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class ZipCompressor: public Compressor
{
  public:

    IMF_EXPORT
    ZipCompressor (const Header &hdr, 
                   size_t maxScanLineSize,
                   size_t numScanLines);

    IMF_EXPORT
    virtual ~ZipCompressor ();

    IMF_EXPORT
    virtual int numScanLines () const;

    IMF_EXPORT
    virtual int	compress (const char *inPtr,
			  int inSize,
			  int minY,
			  const char *&outPtr);

    IMF_EXPORT
    virtual int	uncompress (const char *inPtr,
			    int inSize,
			    int minY,
			    const char *&outPtr);
  private:

    int		_maxScanLineSize;
    int		_numScanLines;
    char *	_outBuffer;
    Zip     _zip;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT





#endif
