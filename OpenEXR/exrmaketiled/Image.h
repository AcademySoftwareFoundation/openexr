#ifndef INCLUDED_IMAGE_H
#define INCLUDED_IMAGE_H

//
//	Copyright  (c)  2004    Industrial   Light   and   Magic.
//	All   rights   reserved.    Used   under   authorization.
//	This material contains the confidential  and  proprietary
//	information   of   Industrial   Light   and   Magic   and
//	may not be copied in whole or in part without the express
//	written   permission   of  Industrial Light  and   Magic.
//	This  copyright  notice  does  not   imply   publication.
//

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


class ImageChannel
{
  public:

    friend class Image;

    ImageChannel (Image &Image);
    virtual ~ImageChannel();

    virtual Imf::Slice	slice () const = 0;

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
    
    Imf::PixelType	pixelType () const;

    virtual Imf::Slice	slice () const;

    T &			operator () (int x, int y);
    const T &		operator () (int x, int y) const;

  private:

    virtual void	resize (int width, int height);

    Imf::Array2D<T>	_pixels;
};


typedef TypedImageChannel<half>		HalfChannel;
typedef TypedImageChannel<float>	FloatChannel;
typedef TypedImageChannel<unsigned int>	UIntChannel;


class Image
{
  public:

    Image ();
    Image (const Imath::Box2i &dataWindow);
   ~Image ();

   const Imath::Box2i &		dataWindow () const;
   void				resize (const Imath::Box2i &dataWindow);

   int				width () const;
   int				height () const;

   void				addChannel (const std::string &name,
					    Imf::PixelType type);

   ImageChannel &		channel (const std::string &name);
   const ImageChannel &		channel (const std::string &name) const;

   template <class T>
   TypedImageChannel<T> &	typedChannel (const std::string &name);

   template <class T>
   const TypedImageChannel<T> &	typedChannel (const std::string &name) const;

  private:

   typedef std::map <std::string, ImageChannel *> ChannelMap;

   Imath::Box2i			_dataWindow;
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
inline Imf::PixelType
HalfChannel::pixelType () const
{
    return Imf::HALF;
}


template <>
inline Imf::PixelType
FloatChannel::pixelType () const
{
    return Imf::FLOAT;
}


template <>
inline Imf::PixelType
UIntChannel::pixelType () const
{
    return Imf::UINT;
}


template <class T>
Imf::Slice
TypedImageChannel<T>::slice () const
{
    const Imath::Box2i &dw = image().dataWindow();
    int w = dw.max.x - dw.min.x + 1;

    return Imf::Slice (pixelType(),
		       (char *) (&_pixels[0][0] - dw.min.y * w - dw.min.x),
		       sizeof (T),
		       w * sizeof (T));
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
    _pixels.resizeErase (height, width);
}


inline const Imath::Box2i &
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
