//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_B44_COMPRESSOR_H
#define INCLUDED_IMF_B44_COMPRESSOR_H

//-----------------------------------------------------------------------------
//
//	class B44Compressor -- lossy compression of 4x4 pixel blocks
//
//-----------------------------------------------------------------------------

#include "ImfCompressor.h"
#include "ImfNamespace.h"
#include "ImfExport.h"
#include "ImfForward.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER


class B44Compressor: public Compressor
{
  public:

    IMF_EXPORT
    B44Compressor (const Header &hdr,
                   size_t maxScanLineSize,
		   size_t numScanLines,
		   bool optFlatFields);

    IMF_EXPORT
    virtual ~B44Compressor ();

    B44Compressor (const B44Compressor& other) = delete;
    B44Compressor& operator = (const B44Compressor& other) = delete;
    B44Compressor (B44Compressor&& other) = delete;
    B44Compressor& operator = (B44Compressor&& other) = delete;
    
    IMF_EXPORT
    virtual int		numScanLines () const;

    IMF_EXPORT
    virtual Format	format () const;

    IMF_EXPORT
    virtual int		compress (const char *inPtr,
				  int inSize,
				  int minY,
				  const char *&outPtr);                  
                  
    IMF_EXPORT
    virtual int		compressTile (const char *inPtr,
				      int inSize,
				      IMATH_NAMESPACE::Box2i range,
				      const char *&outPtr);

    IMF_EXPORT
    virtual int		uncompress (const char *inPtr,
				    int inSize,
				    int minY,
				    const char *&outPtr);
                    
    IMF_EXPORT
    virtual int		uncompressTile (const char *inPtr,
					int inSize,
					IMATH_NAMESPACE::Box2i range,
					const char *&outPtr);
  private:

    struct ChannelData;
    
    int			compress (const char *inPtr,
				  int inSize,
				  IMATH_NAMESPACE::Box2i range,
				  const char *&outPtr);
 
    int			uncompress (const char *inPtr,
				    int inSize,
				    IMATH_NAMESPACE::Box2i range,
				    const char *&outPtr);

    int			_maxScanLineSize;
    bool		_optFlatFields;
    Format		_format;
    int			_numScanLines;
    unsigned short *	_tmpBuffer;
    char *		_outBuffer;
    int			_numChans;
    const ChannelList &	_channels;
    ChannelData *	_channelData;
    int			_minX;
    int			_maxX;
    int			_maxY;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
