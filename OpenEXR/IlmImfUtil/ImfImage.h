///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2014, Industrial Light & Magic, a division of Lucas
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

#ifndef INCLUDED_IMF_IMAGE_H
#define INCLUDED_IMF_IMAGE_H

//----------------------------------------------------------------------------
//
// class Image -- an in-memory data structure that can hold an arbitrary
// OpenEXR image, flat or deep, with one or multiple resolution levels,
// and with an arbitrary set of channels.
// 
// An image is a container for a set of image levels, and an image level
// is a container for a set of image channels.  An image channel contains
// an array of pixel values of type half, float or unsigned int.
// 
// For example:
// 
//     image --+-- level 0 --+-- channel "R" --- pixel data
//             |             |
//             |             +-- channel "G" --- pixel data
//             |             |
//             |             +-- channel "B" --- pixel data
//             |
//             +-- level 1 --+-- channel "R" --- pixel data
//             |             |
//             |             +-- channel "G" --- pixel data
//             |             |
//             |             +-- channel "B" --- pixel data
//             |
//             +-- level 2 --+-- channel "R" --- pixel data
//                           |
//                           +-- channel "G" --- pixel data
//                           |
//                           +-- channel "B" --- pixel data
// 
// An image has a level mode, which can be ONE_LEVEL, MIPMAP_LEVELS or
// RIPMAP_LEVELS, and a level rounding mode, which can be ROUND_UP or
// ROUND_DOWN.  Together, the level mode and the level rounding mode
// determine how many levels an image contains, and how large the data
// window for each level is.  All levels in an image have the same set
// of channels.
// 
// An image channel has a name (e.g. "R", "Z", or "xVelocity"), a type
// (HALF, FLOAT or UINT) and x and y sampling rates.  A channel stores
// samples for a pixel if the pixel is inside the data window of the
// level to which the channel belongs, and the x and y coordinates of
// the pixel are divisible by the x and y sampling rates of the channel.
//
// An image can be either flat or deep.  In a flat image each channel
// in each level stores at most one value per pixel.  In a deep image
// each channel in each level stores an arbitrary number of values per
// pixel.  As an exception, each level of a deep image has a sample count
// channel with a single value per pixel; this value determines how many
// values each of the other channels in the same level has at the same
// pixel location.
//
// The classes Image, ImageLevel and ImageChannel are abstract base
// classes.  Two sets of concrete classes, one for flat and one for
// deep images, are derived from the base classes.
//
//----------------------------------------------------------------------------

#include "ImfImageLevel.h"
#include <ImfTileDescription.h>
#include <ImfArray.h>
#include "ImfExport.h"

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

class Channel;


class IMF_EXPORT Image
{
  public:

    //
    // Constructor and destructor
    //

    Image ();
    virtual ~Image ();


    //
    // Access to the image's level mode and level rounding mode.
    //

	LevelMode               levelMode() const;
	LevelRoundingMode       levelRoundingMode() const;


    //
    // Number of levels:
    //
    // numXLevels() returns the image's number of levels in the x direction.
    //
    //	if levelMode() == ONE_LEVEL:
    //      return value is: 1
    //
    //	if levelMode() == MIPMAP_LEVELS:
    //      return value is: rfunc (log (max (w, h)) / log (2)) + 1
    //
    //	if levelMode() == RIPMAP_LEVELS:
    //      return value is: rfunc (log (w) / log (2)) + 1
    //
    //	where
    //	    w is the width of the image's data window,  max.x - min.x + 1,
    //	    h is the height of the image's data window, max.y - min.y + 1,
    //	    and rfunc(x) is either floor(x), or ceil(x), depending on
    //	    whether levelRoundingMode() returns ROUND_DOWN or ROUND_UP.
    //
    // numYLevels() returns the image's number of levels in the y direction.
    //
    //	if levelMode() == ONE_LEVEL or levelMode() == MIPMAP_LEVELS:
    //      return value is the same as for numXLevels()
    //
    //	if levelMode() == RIPMAP_LEVELS:
    //      return value is: rfunc (log (h) / log (2)) + 1
    //
    //
    // numLevels() is a convenience function for use with MIPMAP_LEVELS images.
    //
    //	if levelMode() == ONE_LEVEL or levelMode() == MIPMAP_LEVELS:
    //      return value is the same as for numXLevels()
    //
    //	if levelMode() == RIPMAP_LEVELS:
    //      a LogicExc exception is thrown
    //

	int                     numLevels() const;
	int                     numXLevels() const;
	int                     numYLevels() const;


    //
    // Per-level data windows
    //
    // dataWindow() returns the data window for the image; this is the
    // same as the data window for the level with level number (0, 0).
    //
    // dataWindowForLevel(lx, ly) returns the data window for level x,
    // that is, the window for which the image level with level number
    // (lx, ly) has allocated pixel storage.
    //
    //	return value is a Box2i with min value:
    //      (dataWindow().min.x,
    //       dataWindow().min.y)
    //
    //	and max value:
    //      (dataWindow().min.x + levelWidth(lx) - 1,
    //       dataWindow().min.y + levelHeight(ly) - 1)
    //
    // dataWindowForLevel(l) is a convenience function used for ONE_LEVEL
    // and MIPMAP_LEVELS files.  It returns dataWindowForLevel(l,l)).
    //

	const IMATH_NAMESPACE::Box2i &  dataWindow() const;
	const IMATH_NAMESPACE::Box2i &  dataWindowForLevel(int l) const;
	const IMATH_NAMESPACE::Box2i &  dataWindowForLevel(int lx, int ly) const;


    //
    // Size of a level:
    //
    // levelWidth(lx) returns the width of a level with level
    // number (lx, *), where * is any number.
    //
    //	return value is:
    //      max (1, rfunc (w / pow (2, lx)))
    //
    //
    // levelHeight(ly) returns the height of a level with level
    // number (*, ly), where * is any number.
    //
    //	return value is:
    //      max (1, rfunc (h / pow (2, ly)))
    //

    int			    levelWidth  (int lx) const;
    int			    levelHeight (int ly) const;


    //
    // Resize the image:
    //
    // resize(dw,lm,lrm) sets the data window of the image to dw,
    // sets the level mode to lm and the level rounding mode to lrm,
    // and allocates new storage for image levels and image channels.
    // The set of channels in the image does not change.
    //
    // The contents of the image are lost; pixel data are not preserved
    // across the resize operation.  If resizing fails, then the image
    // will be left with an empty data window and no image levels.
    //
    // resize(dw) is the same as resize(dw,levelMode(),levelRoundingMode())
    //

	void                    resize(const IMATH_NAMESPACE::Box2i &dataWindow);

	virtual void            resize(const IMATH_NAMESPACE::Box2i &dataWindow,
                                    LevelMode levelMode,
                                    LevelRoundingMode levelRoundingMode);

    //
    // Shift the pixels and the data window of an image:
    //
    // shiftPixels(dx,dy) shifts the image by dx pixels horizontally and
    // dy pixels vertically.  A pixel at location (x,y) moves to position
    // (x+dx, y+dy).  The data window of the image is shifted along with
    // the pixels.  No pixel data are lost.
    //
    // The horizontal and vertical shift distances must be multiples of
    // the x and y sampling rates of all image channels.  If they are not,
    // shiftPixels() throws an ArgExc exception.
    //

	void                    shiftPixels(int dx, int dy);


    //
    // Insert a new channel into the image.
    //
    // The arguments to this function are the same as for adding a
    // a channel to an OpenEXR file: channel name, x and y sampling
    // rates, and a "perceptually approximately linear" flag.
    //
    // If the image already contains a channel with the same name
    // as the new name then the existing channel is deleted before
    // the new channel is added.
    //

    void                    insertChannel (const std::string &name,
                                           PixelType type,
                                           int xSampling = 1,
                                           int ySampling = 1,
                                           bool pLinear = false);

    void                    insertChannel (const std::string &name,
                                           const Channel &channel);

    //
    // Erase channels from an image:
    //
    // eraseChannel(n) erases the channel with name n.
    // clearChannels() erases all channels.
    //

	void                    eraseChannel(const std::string &name);
	void                    clearChannels();


    //
    // Rename an image channel:
    //
    // renameChannel(nOld,nNew) changes the name of the image channel
    // with name nOld to nNew.
    //
    // If the image already contains a channel called nNew, or if the
    // image does not contain a channel called nOld, then renameChannel()
    // throws an ArgExc exception.
    //
    // In the (unlikely) event that renaming the image channel causes
    // the program to run out of memory, renameChannel() erases the
    // channel that is being renamed, and throws an exception.
    //

	void                    renameChannel(const std::string &oldName,
                                           const std::string &newName);

    //
    // Rename multiple image channels at the same time:
    //
    // Given a map, m, from old to new channel names, renameChannels(m)
    // assigns new names to the channels in the image.  If m has an entry
    // for a channel named c, then the channel will be renamed to m[c].
    // If m has no entry for c, then the channel keeps its old name.
    //
    // If the same name would be assigned to more than one channel, then
    // renameChannels() does not rename any channels but throws an ArgExc
    // exception instead.
    // 
    // In the (unlikely) event that renaming the image channel causes the
    // program to run out of memory, renameChannels() erases all channels
    // in the image and throws an exception.
    //

	void                    renameChannels(const RenamingMap &oldToNewNames);


    //
    // Accessing image levels by level number.
    //
    // level(lx,ly) returns a reference to the image level
    // with level number (lx,ly).
    //
    // level(l) returns level(l,l).
    //

    virtual ImageLevel &            level (int l = 0);
    virtual const ImageLevel &      level (int l = 0) const;

    virtual ImageLevel &            level (int lx, int ly);
    virtual const ImageLevel &      level (int lx, int ly) const;


  protected:

    virtual ImageLevel *
        newLevel (int lx, int ly, const IMATH_NAMESPACE::Box2i &dataWindow) = 0;

  private:

    bool        levelNumberIsValid (int lx, int ly) const;
    void        clearLevels ();

    struct ChannelInfo
    {
        ChannelInfo (PixelType type = HALF,
                     int xSampling = 1,
                     int ySampling = 1,
                     bool pLinear = false);

        PixelType   type;
        int         xSampling;
        int         ySampling;
        bool        pLinear;
    };

    typedef std::map <std::string, ChannelInfo> ChannelMap;

    IMATH_NAMESPACE::Box2i  _dataWindow;
    LevelMode               _levelMode;
    LevelRoundingMode       _levelRoundingMode;
    ChannelMap              _channels;
    Array2D<ImageLevel *>   _levels;
};


OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
