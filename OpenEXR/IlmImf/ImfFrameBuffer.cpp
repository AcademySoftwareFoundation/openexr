///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
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
//	class Slice
//	class FrameBuffer
//
//-----------------------------------------------------------------------------

#include <ImfFrameBuffer.h>
#include "Iex.h"
#include "half.h"

using namespace std;

namespace Imf {

Slice::Slice (PixelType t,
	      char *b,
	      size_t xst,
	      size_t yst,
	      int xsm,
	      int ysm,
	      double fv,
              bool xtc,
              bool ytc)
:
    type (t),
    base (b),
    xStride (xst),
    yStride (yst),
    xSampling (xsm),
    ySampling (ysm),
    fillValue (fv),
    xTileCoords (xtc),
    yTileCoords (ytc)
{
    // empty
}


void	
FrameBuffer::insert (const char name[], const Slice &slice)
{
    if (name[0] == 0)
    {
	THROW (Iex::ArgExc,
	       "Frame buffer slice name cannot be an empty string.");
    }

    _map[name] = slice;
}


void	
FrameBuffer::insert (const string &name, const Slice &slice)
{
    insert (name.c_str(), slice);
}


Slice &
FrameBuffer::operator [] (const char name[])
{
    SliceMap::iterator i = _map.find (name);

    if (i == _map.end())
    {
	THROW (Iex::ArgExc,
	       "Cannot find frame buffer slice \"" << name << "\".");
    }

    return i->second;
}


const Slice &
FrameBuffer::operator [] (const char name[]) const
{
    SliceMap::const_iterator i = _map.find (name);

    if (i == _map.end())
    {
	THROW (Iex::ArgExc,
	       "Cannot find frame buffer slice \"" << name << "\".");
    }

    return i->second;
}


Slice &
FrameBuffer::operator [] (const string &name)
{
    return this->operator[] (name.c_str());
}


const Slice &
FrameBuffer::operator [] (const string &name) const
{
    return this->operator[] (name.c_str());
}


Slice *
FrameBuffer::findSlice (const char name[])
{
    SliceMap::iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


const Slice *
FrameBuffer::findSlice (const char name[]) const
{
    SliceMap::const_iterator i = _map.find (name);
    return (i == _map.end())? 0: &i->second;
}


Slice *
FrameBuffer::findSlice (const string &name)
{
    return findSlice (name.c_str());
}


const Slice *
FrameBuffer::findSlice (const string &name) const
{
    return findSlice (name.c_str());
}


FrameBuffer::Iterator
FrameBuffer::begin ()
{
    return _map.begin();
}


FrameBuffer::ConstIterator	
FrameBuffer::begin () const
{
    return _map.begin();
}


FrameBuffer::Iterator
FrameBuffer::end ()
{
    return _map.end();
}


FrameBuffer::ConstIterator	
FrameBuffer::end () const
{
    return _map.end();
}


FrameBuffer::Iterator
FrameBuffer::find (const char name[])
{
    return _map.find (name);
}


FrameBuffer::ConstIterator
FrameBuffer::find (const char name[]) const
{
    return _map.find (name);
}


FrameBuffer::Iterator
FrameBuffer::find (const string &name)
{
    return find (name.c_str());
}


FrameBuffer::ConstIterator
FrameBuffer::find (const string &name) const
{
    return find (name.c_str());
}

OptimizationMode::ChannelsInfo
FrameBuffer::getOptimizationInfo() const
{
    OptimizationMode::ChannelsInfo optimizationInfo;    
    optimizationInfo._format = OptimizationMode::PIXELFORMAT_OTHER;

    int fullMask = 0;
    ConstIterator channelIterator = begin();
    
    if(channelIterator == end())
    {
        return optimizationInfo;
    }

    int globalXStride = channelIterator.slice().xStride;
    
    // Needs to be RGB or RGBA, Mono or Stereo
    if (globalXStride != (3 * sizeof(half)) &&
        globalXStride != (4 * sizeof(half)) &&
        globalXStride != (6 * sizeof(half)) &&
        globalXStride != (8 * sizeof(half)))
    {
        return optimizationInfo;
    }

    // Since we are saving everything contiguously, make sure
    // all the slices have the same ySampling.  We cannot use
    // a different ySampling from the 'R' channel than the 'A' channel
    // because we need the same number of pixels for every channel.
    int globalYSampling = channelIterator.slice().ySampling;

    optimizationInfo._xStride = globalXStride;
    optimizationInfo._ySampling = globalYSampling;

    for (ConstIterator channelIterator = begin(); channelIterator != end(); ++channelIterator)
    {        
        const Slice currentSlice = channelIterator.slice();


        // we support only channels RGB and RGBA for IIF optimizations.  
        // Those values should also be of type HALF.
        // We also support only an xSampling of 1 and the same ySampling
        // across the channels
        if (currentSlice.type != HALF || 
            currentSlice.xStride != globalXStride ||
            currentSlice.ySampling != globalYSampling ||
            currentSlice.xSampling != 1)
        {
            return optimizationInfo;
        }

        // convert the channel name into a string for easy manipulation
        // find out the last element after the last dot
        std::string lChannelName = channelIterator.name();

        int maskForChannel = IIFOptimizable::getMaskFromChannelName (lChannelName);
        fullMask |= maskForChannel;

        if (maskForChannel == IIFOptimizable::CHANNELMASK_A)
        {
            optimizationInfo._alphaFillValueRight = currentSlice.fillValue;
        }
        else if (maskForChannel == IIFOptimizable::CHANNELMASK_ALEFT)
        {
            optimizationInfo._alphaFillValueLeft = currentSlice.fillValue;
        }
    } 

    switch (fullMask)
    {
        case IIFOptimizable::CHANNELMASK_RGB:

            optimizationInfo._format = OptimizationMode::PIXELFORMAT_RGB;
            optimizationInfo._multiview = OptimizationMode::MULTIVIEW_MONO;
            break;

        case IIFOptimizable::CHANNELMASK_RGBA:

            optimizationInfo._format = OptimizationMode::PIXELFORMAT_RGBA;
            optimizationInfo._multiview = OptimizationMode::MULTIVIEW_MONO;
            break;

        case IIFOptimizable::CHANNELMASK_RGB_STEREO:

            optimizationInfo._format = OptimizationMode::PIXELFORMAT_RGB;
            optimizationInfo._multiview = OptimizationMode::MULTIVIEW_STEREO;
            break;

        case IIFOptimizable::CHANNELMASK_RGBA_STEREO:

            optimizationInfo._format = OptimizationMode::PIXELFORMAT_RGBA;
            optimizationInfo._multiview = OptimizationMode::MULTIVIEW_STEREO;
            break;

        default:

            optimizationInfo._format = OptimizationMode::PIXELFORMAT_OTHER;
            break;
    }

    return optimizationInfo;
}


} // namespace Imf
