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

#include <Image.h>

using namespace Imf;
using namespace Imath;
using namespace std;


ImageChannel::ImageChannel (Image &image): _image (image)
{
    // empty
}


ImageChannel::~ImageChannel ()
{
    // empty
}


Image::Image (): _dataWindow (Box2i (V2i (0, 0), V2i (0, 0)))
{
    // empty
}


Image::Image (const Box2i &dataWindow): _dataWindow (dataWindow)
{
    // empty
}


Image::~Image ()
{
    for (ChannelMap::iterator i = _channels.begin(); i != _channels.end(); ++i)
	delete i->second;
}


void			
Image::resize (const Imath::Box2i &dataWindow)
{
    _dataWindow = dataWindow;

    for (ChannelMap::iterator i = _channels.begin(); i != _channels.end(); ++i)
	i->second->resize (width(), height());
}


void
Image::addChannel (const string &name, PixelType type)
{
    switch (type)
    {
      case HALF:
	_channels[name] = new HalfChannel (*this, width(), height());
	break;

      case FLOAT:
	_channels[name] = new FloatChannel (*this, width(), height());
	break;

      case UINT:
	_channels[name] = new UIntChannel (*this, width(), height());
	break;

      default:
	throw Iex::ArgExc ("Unknown channel type.");
    }
}


ImageChannel &
Image::channel (const string &name)
{
    return *_channels.find(name)->second;
}


const ImageChannel &
Image::channel (const string &name) const
{
    return *_channels.find(name)->second;
}
