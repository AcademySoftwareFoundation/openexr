//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMAGE_H
#define INCLUDED_IMAGE_H

//----------------------------------------------------------------------------
//
//	Classes for storing OpenEXR images in memory.
//
//----------------------------------------------------------------------------

#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfArray.h>
#include <ImathBox.h>
#include <half.h>
#include <Iex.h>

#include <string>
#include <map>

#include "namespaceAlias.h"

class Image;


class ImageChannel
{
  public:

    friend class Image;

    ImageChannel (Image &image);
    virtual ~ImageChannel();

    virtual IMF::Slice	slice () const = 0;

    Image &		image ()		{return _image;}
    const Image &	image () const		{return _image;}
    virtual void                         black() =0;
  private:

    virtual void	resize () = 0;

    Image &		_image;
};


template <class T>
class TypedImageChannel: public ImageChannel
{
  public:
    
    TypedImageChannel (Image &image, int xSampling, int ySampling);

    virtual ~TypedImageChannel ();
    
    IMF::PixelType	pixelType () const;

    virtual IMF::Slice	slice () const;
    
    
  private:

    virtual void	resize ();
    virtual void black();
    
    int			_xSampling;
    int			_ySampling;
    IMF::Array2D<T>	_pixels;
};


typedef TypedImageChannel<half>		HalfChannel;
typedef TypedImageChannel<float>	FloatChannel;
typedef TypedImageChannel<unsigned int>	UIntChannel;


class Image
{
  public:

    Image ();
    Image (const IMATH_NAMESPACE::Box2i &dataWindow);
   ~Image ();

    Image (const Image& other) = delete;
    Image & operator = (const Image& other) = delete;
    Image (Image&& other) = delete;
    Image & operator = (Image&& other) = delete;

   const IMATH_NAMESPACE::Box2i &		dataWindow () const;
   void				resize (const IMATH_NAMESPACE::Box2i &dataWindow);
   
   int				width () const;
   int				height () const;

   void				addChannel (const std::string &name,
					    const IMF::Channel &channel);

   ImageChannel &		channel (const std::string &name);
   const ImageChannel &		channel (const std::string &name) const;

   template <class T>
   TypedImageChannel<T> &	typedChannel (const std::string &name);

   template <class T>
   const TypedImageChannel<T> &	typedChannel (const std::string &name) const;

   
   
  private:

   typedef std::map <std::string, ImageChannel *> ChannelMap;

   IMATH_NAMESPACE::Box2i			_dataWindow;
   ChannelMap			_channels;
};


//
// Implementation of templates and inline functions.
//

template <class T>
TypedImageChannel<T>::TypedImageChannel
    (Image &image,
     int xSampling,
     int ySampling)
:
    ImageChannel (image),
    _xSampling (xSampling),
    _ySampling (ySampling),
    _pixels (0, 0)
{
    if ( _xSampling < 1 || _ySampling < 1 )
        throw IEX_NAMESPACE::ArgExc ("Invalid x/y sampling values");
    resize();
}


template <class T>
TypedImageChannel<T>::~TypedImageChannel ()
{
    // empty
}


template <>
inline IMF::PixelType
HalfChannel::pixelType () const
{
    return IMF::HALF;
}


template <>
inline IMF::PixelType
FloatChannel::pixelType () const
{
    return IMF::FLOAT;
}


template <>
inline IMF::PixelType
UIntChannel::pixelType () const
{
    return IMF::UINT;
}


template <class T>
IMF::Slice
TypedImageChannel<T>::slice () const
{
    const IMATH_NAMESPACE::Box2i &dw = image().dataWindow();
    int w = dw.max.x - dw.min.x + 1;

    return IMF::Slice::Make (
        pixelType(),
        &_pixels[0][0],
        dw,
        sizeof(T),
        (w / _xSampling) * sizeof (T),
        _xSampling,
        _ySampling);
}


template <class T>
void
TypedImageChannel<T>::resize ()
{
    int width  = image().width()  / _xSampling;
    int height = image().height() / _ySampling;

    _pixels.resizeEraseUnsafe (height, width);
}


template <class T>
void
TypedImageChannel<T>::black ()
{
    size_t nx = static_cast<size_t>( image().width() ) / static_cast<size_t>( _xSampling );
    size_t ny = static_cast<size_t>( image().height() ) / static_cast<size_t>( _ySampling );
    memset(&_pixels[0][0],0,nx*ny*sizeof(T));
}


inline const IMATH_NAMESPACE::Box2i &
Image::dataWindow () const
{
    return _dataWindow;
}


inline int
Image::width () const
{
    return _dataWindow.max.x - _dataWindow.min.x + 1;
}


inline int
Image::height () const
{
    return _dataWindow.max.y - _dataWindow.min.y + 1;
}


template <class T>
TypedImageChannel<T> &
Image::typedChannel (const std::string &name)
{
    return dynamic_cast <TypedImageChannel<T>&> (channel (name));
}


template <class T>
const TypedImageChannel<T> &
Image::typedChannel (const std::string &name) const
{
    return dynamic_cast <const TypedImageChannel<T>&> (channel (name));
}


#endif
