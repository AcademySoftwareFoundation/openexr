//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_IMAGE_CHANNEL_H
#define INCLUDED_IMF_IMAGE_CHANNEL_H

//----------------------------------------------------------------------------
//
//      class ImageChannel
//
//      For an explanation of images, levels and channels,
//      see the comments in header file Image.h.
//
//----------------------------------------------------------------------------

#include "ImfUtilExport.h"

#include <IexBaseExc.h>
#include <ImfPixelType.h>
#include <ImfFrameBuffer.h>
#include <ImfChannelList.h>
#include <ImathBox.h>
#include <half.h>

#include <typeinfo>
#include <cstring>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class ImageLevel;

//
// Image channels:
//
// An image channel holds the pixel data for a single channel of one level
// of an image.  Separate classes for flat and deep channels are derived
// from the ImageChannel base class.
//

class ImageLevel;

class IMFUTIL_EXPORT_TYPE ImageChannel
{
  public:

    //
    // The OpenEXR pixel type of this channel (HALF, FLOAT or UINT).
    //

    virtual PixelType   pixelType () const = 0;

    //
    // Generate an OpenEXR channel for this image channel.
    //
    
    IMFUTIL_EXPORT
    Channel             channel () const;


    //
    // Access to x and y sampling rates, "perceptually linear" flag,
    // and the number of pixels that are stored in this channel.
    // 

    int                 xSampling () const          {return _xSampling;}
    int                 ySampling () const          {return _ySampling;}
    bool                pLinear () const            {return _pLinear;}
    int                 pixelsPerRow () const       {return _pixelsPerRow;}
    int                 pixelsPerColumn () const    {return _pixelsPerColumn;}
    size_t              numPixels () const          {return _numPixels;}


    //
    // Access to the image level to which this channel belongs.
    //

    ImageLevel &        level ()                    {return _level;}
    const ImageLevel &  level () const              {return _level;}

  protected:

    IMFUTIL_EXPORT
    ImageChannel (ImageLevel &level,
                  int xSampling,
                  int ySampling,
                  bool pLinear);

    IMFUTIL_EXPORT
    virtual ~ImageChannel();

    IMFUTIL_EXPORT
    virtual void        resize ();

    IMFUTIL_EXPORT
	void                boundsCheck(int x, int y) const;

  private:

    ImageChannel (const ImageChannel &);                // not implemented
    ImageChannel & operator = (const ImageChannel &);   // not implemented

    ImageLevel &        _level;
    int                 _xSampling;
    int                 _ySampling;
    bool                _pLinear;
    int                 _pixelsPerRow;
    int                 _pixelsPerColumn;
    size_t              _numPixels;
};

/// \addtogroup TypeConversion
///
/// @{

/// similar to ImfAttribute, we may have type-erased image channels we use internally.
template <typename U>
static U *dynamic_cast_channel (ImageChannel *a)
{
    if (!a)
        return nullptr;
    const auto &aid = typeid(*a);
    const auto &uid = typeid(U);
    // check the fast tests first before comparing names...
    if (aid == uid || !strcmp(aid.name(), uid.name()))
    {
        return static_cast<U *>( a );
    }
    return nullptr;
}
template <typename U>
static const U *dynamic_cast_channel (const ImageChannel *a)
{
    return dynamic_cast_channel <U> ( const_cast <ImageChannel *> ( a ) );
}
template<class U>
static U &dynamic_cast_channel (ImageChannel &a)
{
    U *ret = dynamic_cast_channel <U> (&a);
    if ( ! ret )
        throw IEX_NAMESPACE::TypeExc ("Mismatched image channel type.");
    return *ret;
}
template<class U>
static const U &dynamic_cast_channel (const ImageChannel &a)
{
    const U *ret = dynamic_cast_channel <U> (&a);
    if ( ! ret )
        throw IEX_NAMESPACE::TypeExc ("Mismatched image channel type.");
    return *ret;
}

/// @}


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
