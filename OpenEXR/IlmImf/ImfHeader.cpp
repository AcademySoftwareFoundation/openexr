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
//	class Header
//
//-----------------------------------------------------------------------------

#include <ImfHeader.h>
#include <ImfIO.h>
#include <ImfVersion.h>
#include <ImfCompressor.h>

#include <ImfBoxAttribute.h>
#include <ImfChannelListAttribute.h>
#include <ImfCompressionAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfDoubleAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfOpaqueAttribute.h>
#include <ImfPreviewImageAttribute.h>
#include <ImfStringAttribute.h>
#include <ImfVecAttribute.h>

#include <sstream>

#include <stdlib.h>
#include <time.h>


namespace Imf {

using Imath::Box2i;
using Imath::V2i;
using Imath::V2f;

namespace {


void
staticInitialize ()
{
    static bool initialized = false;

    if (!initialized)
    {
	//
	// One-time initialization -- register
	// some predefined attribute types.
	//
	
	Box2iAttribute::registerAttributeType();
	Box2fAttribute::registerAttributeType();
	ChannelListAttribute::registerAttributeType();
	FloatAttribute::registerAttributeType();
	DoubleAttribute::registerAttributeType();
	IntAttribute::registerAttributeType();
	LineOrderAttribute::registerAttributeType();
	M33fAttribute::registerAttributeType();
	M44fAttribute::registerAttributeType();
	StringAttribute::registerAttributeType();
	V2iAttribute::registerAttributeType();
	V2fAttribute::registerAttributeType();
	V3iAttribute::registerAttributeType();
	V3fAttribute::registerAttributeType();
	PreviewImageAttribute::registerAttributeType();

	initialized = true;
    }
}


void
initialize (Header &header,
	    const Box2i &displayWindow,
	    const Box2i &dataWindow,
	    float pixelAspectRatio,
	    const V2f &screenWindowCenter,
	    float screenWindowWidth,
	    LineOrder lineOrder,
	    Compression compression)
{
    header.insert ("displayWindow", Box2iAttribute (displayWindow));
    header.insert ("dataWindow", Box2iAttribute (dataWindow));
    header.insert ("pixelAspectRatio", FloatAttribute (pixelAspectRatio));
    header.insert ("screenWindowCenter", V2fAttribute (screenWindowCenter));
    header.insert ("screenWindowWidth", FloatAttribute (screenWindowWidth));
    header.insert ("lineOrder", LineOrderAttribute (lineOrder));
    header.insert ("compression", CompressionAttribute (compression));
    header.insert ("channels", ChannelListAttribute ());
}


} // namespace


Header::Header (int width,
		int height,
		float pixelAspectRatio,
		const V2f &screenWindowCenter,
		float screenWindowWidth,
		LineOrder lineOrder,
		Compression compression)
:
    _map()
{
    staticInitialize();

    Box2i displayWindow (V2i (0, 0), V2i (width - 1, height - 1));

    initialize (*this,
		displayWindow,
		displayWindow,
		pixelAspectRatio,
		screenWindowCenter,
		screenWindowWidth,
		lineOrder,
		compression);
}


Header::Header (int width,
		int height,
		const Box2i &dataWindow,
		float pixelAspectRatio,
		const V2f &screenWindowCenter,
		float screenWindowWidth,
		LineOrder lineOrder,
		Compression compression)
:
    _map()
{
    staticInitialize();

    Box2i displayWindow (V2i (0, 0), V2i (width - 1, height - 1));

    initialize (*this,
		displayWindow,
		dataWindow,
		pixelAspectRatio,
		screenWindowCenter,
		screenWindowWidth,
		lineOrder,
		compression);
}


Header::Header (const Box2i &displayWindow,
		const Box2i &dataWindow,
		float pixelAspectRatio,
		const V2f &screenWindowCenter,
		float screenWindowWidth,
		LineOrder lineOrder,
		Compression compression)
:
    _map()
{
    staticInitialize();

    initialize (*this,
		displayWindow,
		dataWindow,
		pixelAspectRatio,
		screenWindowCenter,
		screenWindowWidth,
		lineOrder,
		compression);
}


Header::Header (const Header &other): _map()
{
    for (AttributeMap::const_iterator i = other._map.begin();
	 i != other._map.end();
	 ++i)
    {
	insert (*i->first, *i->second);
    }
}


Header::~Header ()
{
    for (AttributeMap::iterator i = _map.begin();
	 i != _map.end();
	 ++i)
    {
	 delete i->second;
    }
}


Header &		
Header::operator = (const Header &other)
{
    if (this != &other)
    {
	for (AttributeMap::iterator i = _map.begin();
	     i != _map.end();
	     ++i)
	{
	     delete i->second;
	}

	_map.erase (_map.begin(), _map.end());

	for (AttributeMap::const_iterator i = other._map.begin();
	     i != other._map.end();
	     ++i)
	{
	    insert (*i->first, *i->second);
	}
    }

    return *this;
}


void
Header::insert (const char name[], const Attribute &attribute)
{
    if (name[0] == 0)
	THROW (Iex::ArgExc, "Image attribute name cannot be an empty string.");

    AttributeMap::iterator i = _map.find (name);

    if (i == _map.end())
    {
	Attribute *tmp = attribute.copy();

	try
	{
	    _map[name] = tmp;
	}
	catch (...)
	{
	    delete tmp;
	    throw;
	}
    }
    else
    {
	if (strcmp (i->second->typeName(), attribute.typeName()))
	    THROW (Iex::TypeExc, "Cannot assign a value of "
				 "type \"" << attribute.typeName() << "\" "
				 "to image attribute \"" << name << "\" of "
				 "type \"" << i->second->typeName() << "\".");

	Attribute *tmp = attribute.copy();
	delete i->second;
	i->second = tmp;
    }
}


Attribute &		
Header::operator [] (const char name[])
{
    AttributeMap::iterator i = _map.find (name);

    if (i == _map.end())
	THROW (Iex::ArgExc, "Cannot find image attribute \"" << name << "\".");

    return *i->second;
}


const Attribute &	
Header::operator [] (const char name[]) const
{
    AttributeMap::const_iterator i = _map.find (name);

    if (i == _map.end())
	THROW (Iex::ArgExc, "Cannot find image attribute \"" << name << "\".");

    return *i->second;
}


Header::Iterator
Header::begin ()
{
    return _map.begin();
}


Header::ConstIterator
Header::begin () const
{
    return _map.begin();
}


Header::Iterator
Header::end ()
{
    return _map.end();
}


Header::ConstIterator
Header::end () const
{
    return _map.end();
}


Header::Iterator
Header::find (const char name[])
{
    return _map.find (name);
}


Header::ConstIterator
Header::find (const char name[]) const
{
    return _map.find (name);
}


Imath::Box2i &	
Header::displayWindow ()
{
    return static_cast <Box2iAttribute &>
	((*this)["displayWindow"]).value();
}


const Imath::Box2i &
Header::displayWindow () const
{
    return static_cast <const Box2iAttribute &>
	((*this)["displayWindow"]).value();
}

Imath::Box2i &	
Header::dataWindow ()
{
    return static_cast <Box2iAttribute &>
	((*this)["dataWindow"]).value();
}


const Imath::Box2i &
Header::dataWindow () const
{
    return static_cast <const Box2iAttribute &>
	((*this)["dataWindow"]).value();
}


float &		
Header::pixelAspectRatio ()
{
    return static_cast <FloatAttribute &>
	((*this)["pixelAspectRatio"]).value();
}


const float &	
Header::pixelAspectRatio () const
{
    return static_cast <const FloatAttribute &>
	((*this)["pixelAspectRatio"]).value();
}


Imath::V2f &	
Header::screenWindowCenter ()
{
    return static_cast <V2fAttribute &>
	((*this)["screenWindowCenter"]).value();
}


const Imath::V2f &	
Header::screenWindowCenter () const
{
    return static_cast <const V2fAttribute &>
	((*this)["screenWindowCenter"]).value();
}


float &		
Header::screenWindowWidth ()
{
    return static_cast <FloatAttribute &>
	((*this)["screenWindowWidth"]).value();
}


const float &	
Header::screenWindowWidth () const
{
    return static_cast <const FloatAttribute &>
	((*this)["screenWindowWidth"]).value();
}


ChannelList &	
Header::channels ()
{
    return static_cast <ChannelListAttribute &>
	((*this)["channels"]).value();
}


const ChannelList &	
Header::channels () const
{
    return static_cast <const ChannelListAttribute &>
	((*this)["channels"]).value();
}


LineOrder &
Header::lineOrder ()
{
    return static_cast <LineOrderAttribute &>
	((*this)["lineOrder"]).value();
}


const LineOrder &
Header::lineOrder () const
{
    return static_cast <const LineOrderAttribute &>
	((*this)["lineOrder"]).value();
}


Compression &
Header::compression ()
{
    return static_cast <CompressionAttribute &>
	((*this)["compression"]).value();
}


const Compression &
Header::compression () const
{
    return static_cast <const CompressionAttribute &>
	((*this)["compression"]).value();
}


void		
Header::setPreviewImage (const PreviewImage &pi)
{
    insert ("preview", PreviewImageAttribute (pi));
}


PreviewImage &
Header::previewImage ()
{
    return typedAttribute <PreviewImageAttribute> ("preview").value();
}


const PreviewImage &
Header::previewImage () const
{
    return typedAttribute <PreviewImageAttribute> ("preview").value();
}


bool		
Header::hasPreviewImage () const
{
    return findTypedAttribute <PreviewImageAttribute> ("preview") != 0;
}


void		
Header::sanityCheck () const
{
    //
    // The display window and the data window
    // must contain at least one pixel each.
    //

    const Box2i &displayWindow = this->displayWindow();

    if (displayWindow.min.x > displayWindow.max.x ||
	displayWindow.min.y > displayWindow.max.y)
	throw Iex::ArgExc ("Invalid display window in image header.");

    const Box2i &dataWindow = this->dataWindow();

    if (dataWindow.min.x > dataWindow.max.x ||
	dataWindow.min.y > dataWindow.max.y)
	throw Iex::ArgExc ("Invalid data window in image header.");

    //
    // The pixel aspect ratio must be greater than 0.
    // In applications, numbers like the the display or
    // data window dimensions are likely to be multiplied
    // or divided by the pixel aspect ratio; to avoid
    // arithmetic exceptions, we limit the pixel aspect
    // ratio to a range that is smaller than theoretically
    // possible (real aspect ratios are likely to be close
    // to 1.0 anyway).
    //

    float pixelAspectRatio = this->pixelAspectRatio();

    const float MIN_PIXEL_ASPECT_RATIO = 1e-6;
    const float MAX_PIXEL_ASPECT_RATIO = 1e+6;

    if (pixelAspectRatio < MIN_PIXEL_ASPECT_RATIO ||
	pixelAspectRatio > MAX_PIXEL_ASPECT_RATIO)
    {
	throw Iex::ArgExc ("Invalid pixel aspect ratio in image header.");
    }

    //
    // The screen window width must be greater than 0.
    // The size of the screen window can vary over a wide
    // range (fish-eye lens to astronomical telescope),
    // so we can't limit the screen window width to some
    // range like the pixel aspect ratio.
    //

    float screenWindowWidth = this->screenWindowWidth();

    if (screenWindowWidth < 0)
	throw Iex::ArgExc ("Invalid screen window width in image header.");

    //
    // The line order must be one of the predefined values.
    //

    LineOrder lineOrder = this->lineOrder();

    if (lineOrder != INCREASING_Y &&
	lineOrder != DECREASING_Y)
    {
	throw Iex::ArgExc ("Invalid line order in image header.");
    }

    //
    // The compression method must be one of the predefined values.
    //

    if (!isValidCompression (this->compression()))
	throw Iex::ArgExc ("Unknown compression type in image header.");

    //
    // Check the channel list:
    //
    // For each channel, the type must be one of the predefined values,
    // and the x and y coordinates of the data window's corners must be
    // divisible by the x and y subsampling factors.
    //
    // The channel's compression method is not checked here; the check
    // is done when the apropriate compression or decompression routines
    // are selected.
    //

    const ChannelList &channels = this->channels();

    for (ChannelList::ConstIterator i = channels.begin();
	 i != channels.end();
	 ++i)
    {
	const Channel &channel = i.channel();
	
	if (i.channel().type != UINT &&
	    i.channel().type != HALF &&
	    i.channel().type != FLOAT)
	{
	    THROW (Iex::ArgExc, "Pixel type of \"" << i.name() << "\" "
			        "image channel is invalid.");
	}

	if (i.channel().xSampling < 1)
	{
	    THROW (Iex::ArgExc, "The x subsampling factor for the "
			        "\"" << i.name() << "\" channel "
				"is invalid.");
	}

	if (i.channel().ySampling < 1)
	{
	    THROW (Iex::ArgExc, "The y subsampling factor for the "
			        "\"" << i.name() << "\" channel "
				"is invalid.");
	}

	if (dataWindow.min.x % i.channel().xSampling)
	{
	    THROW (Iex::ArgExc, "The minimum x coordinate of the "
			        "image's data window is not a multiple "
			        "of the x subsampling factor of "
			        "the \"" << i.name() << "\" channel.");
	}

	if (dataWindow.min.y % i.channel().ySampling)
	{
	    THROW (Iex::ArgExc, "The minimum y coordinate of the "
			        "image's data window is not a multiple "
			        "of the y subsampling factor of "
			        "the \"" << i.name() << "\" channel.");
	}

	if ((dataWindow.max.x - dataWindow.min.x + 1) % i.channel().xSampling)
	{
	    THROW (Iex::ArgExc, "Number of pixels per row in the "
			        "image's data window is not a multiple "
			        "of the x subsampling factor of "
			        "the \"" << i.name() << "\" channel.");
	}

	if ((dataWindow.max.y - dataWindow.min.y + 1) % i.channel().ySampling)
	{
	    THROW (Iex::ArgExc, "Number of pixels per column in the "
			        "image's data window is not a multiple "
			        "of the y subsampling factor of "
			        "the \"" << i.name() << "\" channel.");
	}
    }
}


long
Header::writeTo (std::ostream &os) const
{
    //
    // Write a "magic number" to identify the file as an image file.
    // Write the current file format version number.
    //

    Xdr::write <StreamIO> (os, MAGIC);
    Xdr::write <StreamIO> (os, VERSION);

    //
    // Write all attributes.  If we have a preview image attribute,
    // keep track of its position in the file.
    //

    long previewPosition = 0;

    const Attribute *preview =
	    findTypedAttribute <PreviewImageAttribute> ("preview");

    for (ConstIterator i = begin(); i != end(); ++i)
    {
	//
	// Write the attribute's name and type.
	//

	Xdr::write <StreamIO> (os, i.name());
	Xdr::write <StreamIO> (os, i.attribute().typeName());

	//
	// Write the size of the attribute value,
	// and the value itself.
	//

	std::ostringstream ss;
	i.attribute().writeValueTo (ss, VERSION);

	std::string s = ss.str();
	Xdr::write <StreamIO> (os, (int) s.length());

	if (&i.attribute() == preview)
	    previewPosition = os.tellp();

	os.write (s.data(), s.length());
    }

    //
    // Write zero-length attribute name to mark the end of the header.
    //

    Xdr::write <StreamIO> (os, "");

    return previewPosition;
}


void
Header::readFrom (std::istream &is, int &version)
{
    //
    // Read the magic number and the file format version number.
    // Then check if we can read the rest of this file.
    //

    int magic;

    Xdr::read <StreamIO> (is, magic);
    Xdr::read <StreamIO> (is, version);

    if (magic != MAGIC)
	throw Iex::InputExc ("File is not an image file.");

    if (version != 1 && version != VERSION)
	THROW (Iex::InputExc, "Cannot read version " << version << " "
			      "image files.  Current file format version "
			      "is " << VERSION << ".");

    //
    // Read all attributes.
    //

    while (true)
    {
	//
	// Read the name of the attribute.
	// A zero-length attribute name indicates the end of the header.
	//

	char name[100];
	Xdr::read <StreamIO> (is, sizeof (name), name);

	if (name[0] == 0)
	    break;

	//
	// Read the attribute type and the size of the attribute value.
	//

	char typeName[100];
	int size;

	Xdr::read <StreamIO> (is, sizeof (typeName), typeName);
	Xdr::read <StreamIO> (is, size);

	AttributeMap::iterator i = _map.find (name);

	if (i != _map.end())
	{
	    //
	    // The attribute already exists (for example,
	    // because it is a predefined attribute).
	    // Read the attribute's new value from the file.
	    //

	    if (strncmp (i->second->typeName(), typeName, sizeof (typeName)))
		THROW (Iex::InputExc, "Unexpected type for image attribute "
				      "\"" << name << "\".");

	    i->second->readValueFrom (is, size, version);
	}
	else
	{
	    //
	    // The new attribute does not exist yet.
	    // If the attribute type is of a known type,
	    // read the attribute value.  If the attribute
	    // is of an unknown type, read its value and
	    // store it as an OpaqueAttribute.
	    //

	    Attribute *attr;

	    if (Attribute::knownType (typeName))
		attr = Attribute::newAttribute (typeName);
	    else
		attr = new OpaqueAttribute (typeName);

	    try
	    {
		attr->readValueFrom (is, size, version);
		_map[name] = attr;
	    }
	    catch (...)
	    {
		delete attr;
		throw;
	    }
	}
    }
}


} // namespace Imf
