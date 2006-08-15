///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
//
//	class B44Compressor
//
//	This compressor is lossy for HALF channels; the compression rate
//	is fixed at 32/16 (approximately 2.6).  FLOAT and UINT channels
//	not compressed; their data are preserved exactly.
//
//	Each HALF channel is split into blocks of 4 by 4 pixels.  An
//	uncompressed block occupies 32 bytes, which are re-interpreted
//	as sixteen 16-bit unsigned integers, s[0] ... s[15].  Compression
//	shrinks the block to 12 bytes.  The compressed 12-byte block
//	contains:
//
//	 - the maximum original pixel value, sMax, rounded to
//	   12-bit precision
//
//	 - a 4-bit shift value, shift
//
//	 - 16 densely packed 5-bit values, d[0] ... d[15], such
//	   that pixel s[i] is approximately equal to
//
//		sMax - (d[i] << shift)
//
//	This compressor can handle positive and negative pixel values,
//	NaNs and infinities are replaced with zeroes before compression.
//
//-----------------------------------------------------------------------------

#include <ImfB44Compressor.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfHuf.h>
#include <ImfWav.h>
#include <ImfMisc.h>
#include <ImathFun.h>
#include <ImathBox.h>
#include <Iex.h>
#include <ImfIO.h>
#include <ImfXdr.h>
#include <ImfAutoArray.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

namespace Imf {

using Imath::divp;
using Imath::modp;
using Imath::Box2i;
using Imath::V2i;
using std::min;

namespace {

//
// Lookup tables for
//	y = exp (x / 8)
// and 
//	x = 8 * log (y)
//

#include "b44ExpLogTable.h"


inline void
convertFromLinear (unsigned short s[16])
{
    for (int i = 0; i < 16; ++i)
	s[i] = expTable[s[i]];
}


inline void
convertToLinear (unsigned short s[16])
{
    for (int i = 0; i < 16; ++i)
	s[i] = logTable[s[i]];
}


void
pack (const unsigned short s[16], unsigned char b[12])
{
    //
    // s[0] ... s[15] represent floating-point numbers in a
    // sign-magnitude format.  Change the representation such
    // that if s[i] is greater than s[j], the floating-point
    // number that corresponds to s[i] is always greater than
    // the floating-point number that corresponds to s[j].
    //
    // Also, replace any bit patterns that represent NaNs or
    // infinities with zeroes.
    //
    //	bit pattern	floating-point		bit pattern
    //	in s[i]		value			in t[i]
    //
    //  0x7bff		+HALF_MAX		0xfbff
    //  0x7bfe					0xfbfe
    //  0x7bfd					0xfbfd
    //	  ...					  ...
    //  0x0002		+2 * HALF_MIN		0x8002
    //  0x0001		+HALF_MIN		0x8001
    //  0x0000		+0.0			0x8000
    //  0x8000		-0.0			0x7fff
    //  0x8001		-HALF_MIN		0x7ffe
    //  0x8002		-2 * HALF_MIN		0x7ffd
    //	  ...					  ...
    //  0xfbfd					0x0f02
    //  0xfbfe					0x0401
    //  0xfbff		-HALF_MAX		0x0400
    //

    unsigned short t[16];

    for (int i = 0; i < 16; ++i)
    {
	if ((s[i] & 0x7c00) == 0x7c00)
	    t[i] = 0x8000;
	else if (s[i] & 0x8000)
	    t[i] = ~s[i];
	else
	    t[i] = s[i] | 0x8000;
    }

    //
    // Find the maximum, tMax, of t[0] ... t[15].
    //

    unsigned short tMax = 0;

    for (int i = 0; i < 16; ++i)
	if (tMax < t[i])
	    tMax = t[i];

    //
    // Round tMax to 12 bits.
    // If tMax is very close to HALF_MAX, then it will be
    // rounded up, producing a bit pattern that corresponds
    // to a NaN or an infinity.  Replace those bit patterns
    // with the one for HALF_MAX, rounded down to 12-bit
    // precision.
    //

    tMax = ((tMax + 0x0008) & 0xfff0);

    if (tMax > 0xfbf0)
	tMax = 0xfbf0;

    //
    // For each t[i], compute the difference, d[i],
    // between t[i] and tMax.  Clamp differences that
    // are negative.  (Negative differences can occur
    // because tMax has been rounded.)
    //
    // Find the maximum, dMax, of d[0] ... d[15].
    //

    unsigned short d[16];
    unsigned short dMax = 0;

    for (int i = 0; i < 16; ++i)
    {
	if (t[i] >= tMax)
	{
	    d[i] = 0;
	}
	else
	{
	    d[i] = tMax - t[i];

	    if (dMax < d[i])
		dMax = d[i];
	}
    }

    //
    // Round d[0] ... d[15] to five-bit precision.
    // Clamp values above 0x1f.
    //

    unsigned char shift = 1;

    while ((dMax >> shift) > 0x1f)
	++shift;

    for (int i = 0; i < 16; ++i)
    {
	d[i] = d[i] + (1 << (shift - 1)) >> shift;

	if (d[i] > 0x1f)
	    d[i] = 0x1f;
    }

    //
    // Pack sMax and d[0] ... d[15] into 12 bytes.
    //

    b[ 0] = (tMax >> 8);
    b[ 1] = (tMax & 0xf0) | shift;
    								// d         b
    b[ 2] = (d[ 0] << 3) | (d[ 1] >> 2);			// 00000111  2
    b[ 3] = (d[ 1] << 6) | (d[ 2] << 1) | (d[ 3] >> 4);		// 11222223  3
    b[ 4] = (d[ 3] << 4) | (d[ 4] >> 1);			// 33334444  4
    b[ 5] = (d[ 4] << 7) | (d[ 5] << 2) | (d[ 6] >> 3);		// 45555566  5
    b[ 6] = (d[ 6] << 5) |  d[ 7];				// 66677777  6
    b[ 7] = (d[ 8] << 3) | (d[ 9] >> 2);			// 88888999  7
    b[ 8] = (d[ 9] << 6) | (d[10] << 1) | (d[11] >> 4);		// 99aaaaab  8
    b[ 9] = (d[11] << 4) | (d[12] >> 1);			// bbbbcccc  9
    b[10] = (d[12] << 7) | (d[13] << 2) | (d[14] >> 3);		// cdddddee 10
    b[11] = (d[14] << 5) |  d[15];				// eeefffff 11
}


inline
void
unpack (const unsigned char b[12], unsigned short s[16])
{
    unsigned short sMax = (b[0] << 8) | (b[1] & 0xf0);
    unsigned char shift = (b[1] & 0x0f);

    s[ 0] = sMax -   ((b[ 2] >> 3)                         << shift);
    s[ 1] = sMax - ((((b[ 2] << 2) | (b[ 3] >> 6)) & 0x1f) << shift);
    s[ 2] = sMax -  (((b[ 3] >> 1)                 & 0x1f) << shift);
    s[ 3] = sMax - ((((b[ 3] << 4) | (b[ 4] >> 4)) & 0x1f) << shift);
    s[ 4] = sMax - ((((b[ 4] << 1) | (b[ 5] >> 7)) & 0x1f) << shift);
    s[ 5] = sMax -  (((b[ 5] >> 2)                 & 0x1f) << shift);
    s[ 6] = sMax - ((((b[ 5] << 3) | (b[ 6] >> 5)) & 0x1f) << shift);
    s[ 7] = sMax -   ((b[ 6]                       & 0x1f) << shift);
    s[ 8] = sMax -   ((b[ 7] >> 3)                         << shift);
    s[ 9] = sMax - ((((b[ 7] << 2) | (b[ 8] >> 6)) & 0x1f) << shift);
    s[10] = sMax -  (((b[ 8] >> 1)                 & 0x1f) << shift);
    s[11] = sMax - ((((b[ 8] << 4) | (b[ 9] >> 4)) & 0x1f) << shift);
    s[12] = sMax - ((((b[ 9] << 1) | (b[10] >> 7)) & 0x1f) << shift);
    s[13] = sMax -  (((b[10] >> 2)                 & 0x1f) << shift);
    s[14] = sMax - ((((b[10] << 3) | (b[11] >> 5)) & 0x1f) << shift);
    s[15] = sMax -   ((b[11]                       & 0x1f) << shift);

    for (int i = 0; i < 16; ++i)
    {
	if (s[i] & 0x8000)
	    s[i] &= 0x7fff;
	else
	    s[i] = ~s[i];
    }
}


void
notEnoughData ()
{
    throw Iex::InputExc ("Error decompressing data "
			 "(input data are shorter than expected).");
}


void
tooMuchData ()
{
    throw Iex::InputExc ("Error decompressing data "
			 "(input data are longer than expected).");
}

} // namespace


struct B44Compressor::ChannelData
{
    unsigned short *	start;
    unsigned short *	end;
    int			nx;
    int			ny;
    int			ys;
    PixelType		type;
    bool		pLinear;
    int			size;
};


B44Compressor::B44Compressor
    (const Header &hdr,
     int maxScanLineSize,
     int numScanLines)
:
    Compressor (hdr),
    _maxScanLineSize (maxScanLineSize),
    _format (XDR),
    _numScanLines (numScanLines),
    _tmpBuffer (0),
    _outBuffer (0),
    _numChans (0),
    _channels (hdr.channels()),
    _channelData (0)
{
    //
    // Allocate buffers for compressed an uncompressed pixel data,
    // allocate a set of ChannelData structs to help speed up the
    // compress() and uncompress() functions, below, and determine
    // if uncompressed pixel data should be in native or Xdr format.
    //

    _tmpBuffer = new unsigned short [maxScanLineSize * numScanLines];

    const ChannelList &channels = header().channels();
    int numHalfChans = 0;

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c)
    {
	assert (pixelTypeSize (c.channel().type) % pixelTypeSize (HALF) == 0);
	++_numChans;

	if (c.channel().type == HALF)
	    ++numHalfChans;
    }

    //
    // Compressed data may be larger than the input data
    //

    int padding = 12 * numHalfChans * (numScanLines + 3) / 4;

    _outBuffer = new char [maxScanLineSize * numScanLines + padding];
    _channelData = new ChannelData[_numChans];

    int i = 0;

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c, ++i)
    {
	_channelData[i].ys = c.channel().ySampling;
	_channelData[i].type = c.channel().type;
	_channelData[i].pLinear = c.channel().pLinear;
	_channelData[i].size =
	    pixelTypeSize (c.channel().type) / pixelTypeSize (HALF);
    }

    const Box2i &dataWindow = hdr.dataWindow();

    _minX = dataWindow.min.x;
    _maxX = dataWindow.max.x;
    _maxY = dataWindow.max.y;

    //
    // We can support uncompressed data in the machine's native
    // format only if all image channels are of type HALF.
    //

    assert (sizeof (unsigned short) == pixelTypeSize (HALF));

    if (_numChans == numHalfChans)
	_format = NATIVE;
}


B44Compressor::~B44Compressor ()
{
    delete [] _tmpBuffer;
    delete [] _outBuffer;
    delete [] _channelData;
}


int
B44Compressor::numScanLines () const
{
    return _numScanLines;
}


Compressor::Format
B44Compressor::format () const
{
    return _format;
}


int
B44Compressor::compress (const char *inPtr,
			 int inSize,
			 int minY,
			 const char *&outPtr)
{
    return compress (inPtr,
		     inSize,
		     Box2i (V2i (_minX, minY),
			    V2i (_maxX, minY + numScanLines() - 1)),
		     outPtr);
}


int
B44Compressor::compressTile (const char *inPtr,
			     int inSize,
			     Imath::Box2i range,
			     const char *&outPtr)
{
    return compress (inPtr, inSize, range, outPtr);
}


int
B44Compressor::uncompress (const char *inPtr,
			   int inSize,
			   int minY,
			   const char *&outPtr)
{
    return uncompress (inPtr,
		       inSize,
		       Box2i (V2i (_minX, minY),
			      V2i (_maxX, minY + numScanLines() - 1)),
		       outPtr);
}


int
B44Compressor::uncompressTile (const char *inPtr,
			       int inSize,
			       Imath::Box2i range,
			       const char *&outPtr)
{
    return uncompress (inPtr, inSize, range, outPtr);
}


int
B44Compressor::compress (const char *inPtr,
			 int inSize,
			 Imath::Box2i range,
			 const char *&outPtr)
{
    //
    // Compress a block of pixel data:  First copy the input pixels
    // from the input buffer into _tmpBuffer, rearranging them such
    // that blocks of 4x4 pixels of a singe channel can be accessed
    // conveniently.  Then compress each 4x4 block of HALF pixel data
    // and append the result to the output buffer.  Copy UINT and
    // FLOAT data to the output buffer without compressing them.
    //

    outPtr = _outBuffer;

    if (inSize == 0)
    {
	//
	// Special case - empty input buffer.
	//

	return 0;
    }

    //
    // For each channel, detemine how many pixels are stored
    // in the input buffer, and where those pixels will be
    // placed in _tmpBuffer.
    //

    int minX = range.min.x;
    int maxX = min (range.max.x, _maxX);
    int minY = range.min.y;
    int maxY = min (range.max.y, _maxY);
    
    unsigned short *tmpBufferEnd = _tmpBuffer;
    int i = 0;

    for (ChannelList::ConstIterator c = _channels.begin();
	 c != _channels.end();
	 ++c, ++i)
    {
	ChannelData &cd = _channelData[i];

	cd.start = tmpBufferEnd;
	cd.end = cd.start;

	cd.nx = numSamples (c.channel().xSampling, minX, maxX);
	cd.ny = numSamples (c.channel().ySampling, minY, maxY);

	tmpBufferEnd += cd.nx * cd.ny * cd.size;
    }

    if (_format == XDR)
    {
	//
	// The data in the input buffer are in the machine-independent
	// Xdr format.  Copy the HALF channels into _tmpBuffer and
	// convert them back into native format for compression.
	// Copy UINT and FLOAT channels verbatim into _tmpBuffer.
	//

	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		if (modp (y, cd.ys) != 0)
		    continue;

		if (cd.type == HALF)
		{
		    for (int x = cd.nx; x > 0; --x)
		    {
			Xdr::read <CharPtrIO> (inPtr, *cd.end);
			++cd.end;
		    }
		}
		else
		{
		    int n = cd.nx * cd.size;
		    memcpy (cd.end, inPtr, n * sizeof (unsigned short));
		    inPtr += n * sizeof (unsigned short);
		    cd.end += n;
		}
	    }
	}
    }
    else
    {
	//
	// The input buffer contains only HALF channels, and they
	// are in native, machine-dependent format.  Copy the pixels
	// into _tmpBuffer.
	//

	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		#if defined (DEBUG)
		    assert (cd.type == HALF);
		#endif

		if (modp (y, cd.ys) != 0)
		    continue;

		int n = cd.nx * cd.size;
		memcpy (cd.end, inPtr, n * sizeof (unsigned short));
		inPtr  += n * sizeof (unsigned short);
		cd.end += n;
	    }
	}
    }

    //
    // The pixels for each channel have been packed into a contiguous
    // block in _tmpBuffer.  HALF channels are in native format; UINT
    // and FLOAT channels are in Xdr format.
    //

    #if defined (DEBUG)

	for (int i = 1; i < _numChans; ++i)
	    assert (_channelData[i-1].end == _channelData[i].start);

	assert (_channelData[_numChans-1].end == tmpBufferEnd);

    #endif

    //
    // For each HALF channel, split the data in _tmpBuffer into 4x4
    // pixel blocks.  Compress each block and append the compressed
    // data to the output buffer.
    //
    // UINT and FLOAT channels are copied from _tmpBuffer into the
    // output buffer without further processing.
    //

    char *outEnd = _outBuffer;

    for (int i = 0; i < _numChans; ++i)
    {
	ChannelData &cd = _channelData[i];
	
	if (cd.type != HALF)
	{
	    //
	    // UINT or FLOAT channel.
	    //

	    int n = cd.nx * cd.ny * cd.size * sizeof (unsigned short);
	    memcpy (outEnd, cd.start, n);
	    outEnd += n;

	    continue;
	}
	
	//
	// HALF channel
	//

	for (int y = 0; y < cd.ny; y += 4)
	{
	    //
	    // Copy the next 4x4 pixel block into array s.
	    // If the width, cd.nx, or the height, cd.ny, of
	    // the pixel data in _tmpBuffer is not divisible
	    // by 4, then pad the data by repeating the
	    // rightmost column and the bottom row.
	    // 

	    unsigned short *row0 = cd.start + y * cd.nx;
	    unsigned short *row1 = row0 + cd.nx;
	    unsigned short *row2 = row1 + cd.nx;
	    unsigned short *row3 = row2 + cd.nx;

	    if (y + 3 >= cd.ny)
	    {
		if (y + 1 >= cd.ny)
		    row1 = row0;

		if (y + 2 >= cd.ny)
		    row2 = row1;

		row3 = row2;
	    }

	    for (int x = 0; x < cd.nx; x += 4)
	    {
		unsigned short s[16];

		if (x + 3 >= cd.nx)
		{
		    int n = cd.nx - x;

		    for (int i = 0; i < 4; ++i)
		    {
			int j = min (i, n - 1);

			s[i +  0] = row0[j];
			s[i +  4] = row1[j];
			s[i +  8] = row2[j];
			s[i + 12] = row3[j];
		    }
		}
		else
		{
		    memcpy (&s[ 0], row0, 4 * sizeof (unsigned short));
		    memcpy (&s[ 4], row1, 4 * sizeof (unsigned short));
		    memcpy (&s[ 8], row2, 4 * sizeof (unsigned short));
		    memcpy (&s[12], row3, 4 * sizeof (unsigned short));
		}

		row0 += 4;
		row1 += 4;
		row2 += 4;
		row3 += 4;

		//
		// Compress the contents of array s and append the
		// results to the output buffer.
		//

		if (cd.pLinear)
		    convertFromLinear (s);

		pack (s, (unsigned char *) outEnd);
		outEnd += 12;
	    }
	}
    }

    return outEnd - _outBuffer;
}


int
B44Compressor::uncompress (const char *inPtr,
			   int inSize,
			   Imath::Box2i range,
			   const char *&outPtr)
{
    //
    // This function is the reverse of the compress() function,
    // above.  First all pixels are moved from the input buffer
    // into _tmpBuffer.  UINT and FLOAT channels are copied
    // verbatim; HALF channels are uncompressed in blocks of
    // 4x4 pixels.  Then the pixels in _tmpBuffer are copied
    // into the output buffer and rearranged such that the data
    // for for each scan line form a contiguous block.
    //

    outPtr = _outBuffer;

    if (inSize == 0)
    {
	return 0;
    }

    int minX = range.min.x;
    int maxX = min (range.max.x, _maxX);
    int minY = range.min.y;
    int maxY = min (range.max.y, _maxY);
    
    unsigned short *tmpBufferEnd = _tmpBuffer;
    int i = 0;

    for (ChannelList::ConstIterator c = _channels.begin();
	 c != _channels.end();
	 ++c, ++i)
    {
	ChannelData &cd = _channelData[i];

	cd.start = tmpBufferEnd;
	cd.end = cd.start;

	cd.nx = numSamples (c.channel().xSampling, minX, maxX);
	cd.ny = numSamples (c.channel().ySampling, minY, maxY);

	tmpBufferEnd += cd.nx * cd.ny * cd.size;
    }

    for (int i = 0; i < _numChans; ++i)
    {
	ChannelData &cd = _channelData[i];

	if (cd.type != HALF)
	{
	    //
	    // UINT or FLOAT channel.
	    //

	    int n = cd.nx * cd.ny * cd.size * sizeof (unsigned short);

	    if (inSize < n)
		notEnoughData();

	    memcpy (cd.start, inPtr, n);
	    inPtr += n;
	    inSize -= n;

	    continue;
	}

	//
	// HALF channel
	//

	for (int y = 0; y < cd.ny; y += 4)
	{
	    unsigned short *row0 = cd.start + y * cd.nx;
	    unsigned short *row1 = row0 + cd.nx;
	    unsigned short *row2 = row1 + cd.nx;
	    unsigned short *row3 = row2 + cd.nx;

	    for (int x = 0; x < cd.nx; x += 4)
	    {
		if (inSize < 12)
		    notEnoughData();

		unsigned short s[16]; 
		unpack ((const unsigned char *)inPtr, s);
		inPtr += 12;
		inSize -= 12;

		if (cd.pLinear)
		    convertToLinear (s);

		int n = (x + 3 < cd.nx)?
			    4 * sizeof (unsigned short) :
			    (cd.nx - x) * sizeof (unsigned short);

		if (y + 3 < cd.ny)
		{
		    mempcpy (row0, &s[ 0], n);
		    mempcpy (row1, &s[ 4], n);
		    mempcpy (row2, &s[ 8], n);
		    mempcpy (row3, &s[12], n);
		}
		else
		{
		    mempcpy (row0, &s[ 0], n);

		    if (y + 1 < cd.ny)
			mempcpy (row1, &s[ 4], n);

		    if (y + 2 < cd.ny)
			mempcpy (row2, &s[ 8], n);
		}

		row0 += 4;
		row1 += 4;
		row2 += 4;
		row3 += 4;
	    }
	}
    }

    char *outEnd = _outBuffer;

    if (_format == XDR)
    {
	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		if (modp (y, cd.ys) != 0)
		    continue;

		if (cd.type == HALF)
		{
		    for (int x = cd.nx; x > 0; --x)
		    {
			Xdr::write <CharPtrIO> (outEnd, *cd.end);
			++cd.end;
		    }
		}
		else
		{
		    int n = cd.nx * cd.size;
		    memcpy (outEnd, cd.end, n * sizeof (unsigned short));
		    outEnd += n * sizeof (unsigned short);
		    cd.end += n;
		}
	    }
	}
    }
    else
    {
	for (int y = minY; y <= maxY; ++y)
	{
	    for (int i = 0; i < _numChans; ++i)
	    {
		ChannelData &cd = _channelData[i];

		#if defined (DEBUG)
		    assert (cd.type == HALF);
		#endif

		if (modp (y, cd.ys) != 0)
		    continue;

		int n = cd.nx * cd.size;
		memcpy (outEnd, cd.end, n * sizeof (unsigned short));
		outEnd += n * sizeof (unsigned short);
		cd.end += n;
	    }
	}
    }

    #if defined (DEBUG)

	for (int i = 1; i < _numChans; ++i)
	    assert (_channelData[i-1].end == _channelData[i].start);

	assert (_channelData[_numChans-1].end == tmpBufferEnd);

    #endif

    if (inSize > 0)
	tooMuchData();

    outPtr = _outBuffer;
    return outEnd - _outBuffer;
}


} // namespace Imf
