//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//----------------------------------------------------------------------------
//
//	Classes for storing OpenEXR images in memory.
//
//----------------------------------------------------------------------------

#include "Image.h"
#include "namespaceAlias.h"
#include <Iex.h>

using namespace IMF;
using namespace IMATH_NAMESPACE;
using namespace std;

ImageChannel::ImageChannel (Image& image) : _image (image)
{
    // empty
}

ImageChannel::~ImageChannel ()
{
    // empty
}

Image::Image () : _dataWindow (Box2i (V2i (0, 0), V2i (0, 0)))
{
    // empty
}

Image::Image (const Box2i& dataWindow) : _dataWindow (dataWindow)
{
    // empty
}

Image::~Image ()
{
    for (ChannelMap::iterator i = _channels.begin (); i != _channels.end ();
         ++i)
        delete i->second;
}

void
Image::resize (const IMATH_NAMESPACE::Box2i& dataWindow)
{
    _dataWindow = dataWindow;

    for (ChannelMap::iterator i = _channels.begin (); i != _channels.end ();
         ++i)
        i->second->resize (width (), height ());
}

void
Image::addChannel (const string& name, PixelType type)
{
    switch (type)
    {
        case IMF::HALF:
            _channels[name] = new HalfChannel (*this, width (), height ());
            break;

        case IMF::FLOAT:
            _channels[name] = new FloatChannel (*this, width (), height ());
            break;

        case IMF::UINT:
            _channels[name] = new UIntChannel (*this, width (), height ());
            break;

        default: throw IEX_NAMESPACE::ArgExc ("Unknown channel type.");
    }
}

ImageChannel&
Image::channel (const string& name)
{
    return *_channels.find (name)->second;
}

const ImageChannel&
Image::channel (const string& name) const
{
    return *_channels.find (name)->second;
}
