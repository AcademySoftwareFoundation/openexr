///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
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
//	Miscellaneous image file related stuff
//
//-----------------------------------------------------------------------------

#include <ImfMisc.h>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfXdr.h>
#include <ImathFun.h>
#include <Iex.h>

namespace Imf {

using Imath::Box2i;
using Imath::modp;
using std::vector;


int
pixelTypeSize (PixelType type)
{
    int size;

    switch (type)
    {
      case UINT:
	
	size = Xdr::size <unsigned int> ();
	break;

      case HALF:

	size = Xdr::size <half> ();
	break;

      case FLOAT:

	size = Xdr::size <float> ();
	break;

      default:

	throw Iex::ArgExc ("Unknown pixel type.");
    }

    return size;
}


size_t
bytesPerLineTable (const Header &header,
		   vector<size_t> &bytesPerLine)
{
    const Box2i &dataWindow = header.dataWindow();
    const ChannelList &channels = header.channels();

    bytesPerLine.resize (dataWindow.max.y - dataWindow.min.y + 1);

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c)
    {
	int nBytes = pixelTypeSize (c.channel().type) *
		     (dataWindow.max.x - dataWindow.min.x + 1) /
		     c.channel().xSampling;

	for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	    if (modp (y, c.channel().ySampling) == 0)
		bytesPerLine[i] += nBytes;
    }

    size_t maxBytesPerLine = 0;

    for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	if (maxBytesPerLine < bytesPerLine[i])
	    maxBytesPerLine = bytesPerLine[i];

    return maxBytesPerLine;
}


void
offsetInLineBufferTable (const vector<size_t> &bytesPerLine,
			 int linesInLineBuffer,
			 vector<size_t> &offsetInLineBuffer)
{
    offsetInLineBuffer.resize (bytesPerLine.size());

    size_t offset = 0;

    for (int i = 0; i < bytesPerLine.size(); ++i)
    {
	if (i % linesInLineBuffer == 0)
	    offset = 0;

	offsetInLineBuffer[i] = offset;
	offset += bytesPerLine[i];
    }
}


int
lineBufferMinY (int y, int minY, int linesInLineBuffer)
{
    return ((y - minY) / linesInLineBuffer) * linesInLineBuffer + minY;
}


int
lineBufferMaxY (int y, int minY, int linesInLineBuffer)
{
    return lineBufferMinY (y, minY, linesInLineBuffer) + linesInLineBuffer - 1;
}


} // namespace Imf
