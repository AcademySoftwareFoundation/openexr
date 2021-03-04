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
#include "ImfNamespace.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class RleCompressor: public Compressor
{
  public:

    IMF_EXPORT
    RleCompressor (const Header &hdr, size_t maxScanLineSize);
    IMF_EXPORT
    virtual ~RleCompressor ();

    RleCompressor (const RleCompressor& other) = delete;
    RleCompressor& operator = (const RleCompressor& other) = delete;
    RleCompressor (RleCompressor&& other) = delete;
    RleCompressor& operator = (RleCompressor&& other) = delete;

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
    char *	_tmpBuffer;
    char *	_outBuffer;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
