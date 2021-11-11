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

#include <ImfPixelType.h>
#include <ImfFrameBuffer.h>
#include <ImfArray.h>
#include <ImathBox.h>
#include <half.h>

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

  private:

    virtual void	resize (int width, int height) = 0;

    Image &		_image;
};


template <class T>
class TypedImageChannel: public ImageChannel
{
  public:
    
    TypedImageChannel (Image &image, int width, int height);
    virtual ~TypedImageChannel ();
    
    IMF::PixelType	pixelType () const;

    virtual IMF::Slice	slice () const;

    T &				operator () (int x, int y);
    const T &			operator () (int x, int y) const;

  private:

    virtual void		resize (int width, int height);

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
    
    const IMATH_NAMESPACE::Box2i &	dataWindow () const;
    void			resize (const IMATH_NAMESPACE::Box2i &dataWindow);

    int				width () const;
    int				height () const;

    void			addChannel (const std::string &name,
        			            IMF::PixelType type);

    ImageChannel &		channel (const std::string &name);
    const ImageChannel &		channel (const std::string &name) const;

    template <class T>
    TypedImageChannel<T> &	typedChannel (const std::string &name);

    template <class T>
    const TypedImageChannel<T> &	typedChannel (const std::string &name) const;

  private:

    typedef std::map <std::string, ImageChannel *> ChannelMap;

    IMATH_NAMESPACE::Box2i		_dataWindow;
    ChannelMap			_channels;
};


//
// Implementation of templates and inline functions.
//

template <class T>
TypedImageChannel<T>::TypedImageChannel (Image &image, int width, int height):
    ImageChannel (image),
    _pixels (height, width)
{
    // empty
}


template <class T>
TypedImageChannel<T>::~TypedImageChannel ()
{
    // empty
}


template <>
inline OPENEXR_IMF_INTERNAL_NAMESPACE::PixelType
HalfChannel::pixelType () const
{
    return OPENEXR_IMF_INTERNAL_NAMESPACE::HALF;
}


template <>
inline OPENEXR_IMF_INTERNAL_NAMESPACE::PixelType
FloatChannel::pixelType () const
{
    return OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT;
}


template <>
inline OPENEXR_IMF_INTERNAL_NAMESPACE::PixelType
UIntChannel::pixelType () const
{
    return OPENEXR_IMF_INTERNAL_NAMESPACE::UINT;
}


template <class T>
OPENEXR_IMF_INTERNAL_NAMESPACE::Slice
TypedImageChannel<T>::slice () const
{
    const IMATH_NAMESPACE::Box2i &dw = image().dataWindow();

    return OPENEXR_IMF_INTERNAL_NAMESPACE::Slice::Make (
        pixelType(),
        &_pixels[0][0],
        dw,
        sizeof (T));
}


template <class T>
inline const T &
TypedImageChannel<T>::operator () (int x, int y) const
{
    return _pixels[y][x];
}


template <class T>
inline T &
TypedImageChannel<T>::operator () (int x, int y)
{
    return _pixels[y][x];
}


template <class T>
void
TypedImageChannel<T>::resize (int width, int height)
{
    _pixels.resizeEraseUnsafe (height, width);
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
