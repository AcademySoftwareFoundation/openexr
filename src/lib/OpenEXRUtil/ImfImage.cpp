//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//----------------------------------------------------------------------------
//
//      class Image
//
//----------------------------------------------------------------------------

#include "ImfImage.h"
#include <Iex.h>
#include <ImfChannelList.h>
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>

using namespace IMATH_NAMESPACE;
using namespace IEX_NAMESPACE;
using namespace std;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace
{

int64_t
pixelExtent (int64_t minCoord, int64_t maxCoord)
{
    if (maxCoord < minCoord) return 0;

    return maxCoord - minCoord + 1;
}

void
validateDataWindow (const Box2i64& dw)
{
    if (dw.min.x > dw.max.x + 1 || dw.min.y > dw.max.y + 1)
    {
        THROW (ArgExc, "Invalid data window for image resize.");
    }

    if (dw.min.x <= -(INT_MAX / 2) || dw.min.y <= -(INT_MAX / 2) ||
        dw.max.x >= (INT_MAX / 2) || dw.max.y >= (INT_MAX / 2))
    {
        THROW (ArgExc, "Invalid data window for image resize.");
    }

    const int64_t w = pixelExtent (dw.min.x, dw.max.x);
    const int64_t h = pixelExtent (dw.min.y, dw.max.y);

    if (w < 0 || h < 0 || w > INT_MAX || h > INT_MAX)
    {
        THROW (ArgExc, "Invalid data window for image resize.");
    }
}

int
levelSize (
    int64_t           minCoord,
    int64_t           maxCoord,
    int               l,
    LevelRoundingMode levelRoundingMode)
{
    assert (l >= 0);

    int64_t a = pixelExtent (minCoord, maxCoord);
    if (a == 0) return 0;

    if (a > INT_MAX)
    {
        THROW (ArgExc, "Invalid level size for image resize.");
    }

    int64_t b    = int64_t (1) << l;
    int64_t size = a / b;

    if (levelRoundingMode == ROUND_UP && size * b < a) size += 1;

    size = std::max (size, int64_t (1));

    if (size > INT_MAX)
    {
        THROW (ArgExc, "Invalid level size for image resize.");
    }

    return int (size);
}

int
floorLog2 (int64_t x)
{
    //
    // For x > 0, floorLog2(y) returns floor(log(x)/log(2)).
    //

    int y = 0;

    while (x > 1)
    {
        y += 1;
        x >>= 1;
    }

    return y;
}

int
ceilLog2 (int64_t x)
{
    //
    // For x > 0, ceilLog2(y) returns ceil(log(x)/log(2)).
    //

    int y = 0;
    int r = 0;

    while (x > 1)
    {
        if (x & 1) r = 1;

        y += 1;
        x >>= 1;
    }

    return y + r;
}

int
roundLog2 (int64_t x, LevelRoundingMode levelRoundingMode)
{
    if (x < 1) return 1;

    return (levelRoundingMode == ROUND_DOWN) ? floorLog2 (x) : ceilLog2 (x);
}

int
computeNumXLevels (
    const Box2i64&    dw,
    LevelMode         levelMode,
    LevelRoundingMode levelRoundingMode)
{
    const int64_t w = pixelExtent (dw.min.x, dw.max.x);
    const int64_t h = pixelExtent (dw.min.y, dw.max.y);

    int n = 0;

    switch (levelMode)
    {
        case ONE_LEVEL: n = 1; break;

        case MIPMAP_LEVELS:
            n = roundLog2 (std::max (w, h), levelRoundingMode) + 1;
            break;

        case RIPMAP_LEVELS:
            n = roundLog2 (w, levelRoundingMode) + 1;
            break;

        default: assert (false);
    }

    return n;
}

int
computeNumYLevels (
    const Box2i64&    dw,
    LevelMode         levelMode,
    LevelRoundingMode levelRoundingMode)
{
    const int64_t w = pixelExtent (dw.min.x, dw.max.x);
    const int64_t h = pixelExtent (dw.min.y, dw.max.y);

    int n = 0;

    switch (levelMode)
    {
        case ONE_LEVEL: n = 1; break;

        case MIPMAP_LEVELS:
            n = roundLog2 (std::max (w, h), levelRoundingMode) + 1;
            break;

        case RIPMAP_LEVELS:
            n = roundLog2 (h, levelRoundingMode) + 1;
            break;

        default: assert (false);
    }

    return n;
}

} // namespace

Image::Image ()
    : _dataWindow (Box2i (V2i (0, 0), V2i (-1, -1)))
    , _levelMode (ONE_LEVEL)
    , _levelRoundingMode (ROUND_DOWN)
    , _channels ()
    , _levels ()
{
    // empty
}

Image::~Image ()
{
    clearLevels ();
    clearChannels ();
}

LevelMode
Image::levelMode () const
{
    return _levelMode;
}

LevelRoundingMode
Image::levelRoundingMode () const
{
    return _levelRoundingMode;
}

int
Image::numLevels () const
{
    if (_levelMode == ONE_LEVEL || _levelMode == MIPMAP_LEVELS)
        return numXLevels ();
    else
        throw LogicExc ("Number of levels query for image "
                        "must specify x or y direction.");
}

int
Image::numXLevels () const
{
    return _levels.width ();
}

int
Image::numYLevels () const
{
    return _levels.height ();
}

const Box2i&
Image::dataWindow () const
{
    return _dataWindow;
}

const Box2i&
Image::dataWindowForLevel (int l) const
{
    return dataWindowForLevel (l, l);
}

const Box2i&
Image::dataWindowForLevel (int lx, int ly) const
{
    if (!levelNumberIsValid (lx, ly))
    {
        THROW (
            ArgExc,
            "Cannot get data window for invalid image "
            "level ("
                << lx << ", " << ly << ").");
    }

    return _levels[ly][lx]->dataWindow ();
}

int
Image::levelWidth (int lx) const
{
    if (lx < 0 || lx >= numXLevels ())
    {
        THROW (
            ArgExc,
            "Cannot get level width for invalid "
            "image level number "
                << lx << ".");
    }

    return levelSize (_dataWindow.min.x, _dataWindow.max.x, lx, _levelRoundingMode);
}

int
Image::levelHeight (int ly) const
{
    if (ly < 0 || ly >= numYLevels ())
    {
        THROW (
            ArgExc,
            "Cannot get level height for invalid "
            "image level number "
                << ly << ".");
    }

    return levelSize (_dataWindow.min.y, _dataWindow.max.y, ly, _levelRoundingMode);
}

void
Image::resize (const Box2i& dataWindow)
{
    resize (dataWindow, _levelMode, _levelRoundingMode);
}

void
Image::resize (
    const Box2i&      dataWindow,
    LevelMode         levelMode,
    LevelRoundingMode levelRoundingMode)
{
    try
    {
        clearLevels ();

        const Box2i64 dw (dataWindow.min, dataWindow.max);

        validateDataWindow (dw);

        int nx = computeNumXLevels (dw, levelMode, levelRoundingMode);
        int ny = computeNumYLevels (dw, levelMode, levelRoundingMode);

        _levels.resizeErase (ny, nx);

        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
                _levels[y][x] = nullptr;

        for (int y = 0; y < ny; ++y)
        {
            for (int x = 0; x < nx; ++x)
            {
                if (levelMode == MIPMAP_LEVELS && x != y)
                {
                    _levels[y][x] = 0;
                    continue;
                }

                const V2i ls (levelSize (dw.min.x, dw.max.x, x, levelRoundingMode) - 1,
                              levelSize (dw.min.y, dw.max.y, y, levelRoundingMode) - 1);
                const Box2i levelDataWindow (dataWindow.min, dataWindow.min + ls);

                _levels[y][x] = newLevel (x, y, levelDataWindow);

                for (ChannelMap::iterator i = _channels.begin ();
                     i != _channels.end ();
                     ++i)
                {
                    _levels[y][x]->insertChannel (
                        i->first,
                        i->second.type,
                        i->second.xSampling,
                        i->second.ySampling,
                        i->second.pLinear);
                }
            }
        }

        _dataWindow        = dataWindow;
        _levelMode         = levelMode;
        _levelRoundingMode = levelRoundingMode;
    }
    catch (...)
    {
        clearLevels ();
        throw;
    }
}

void
Image::shiftPixels (int dx, int dy)
{
    for (ChannelMap::iterator i = _channels.begin (); i != _channels.end ();
         ++i)
    {
        if (dx % i->second.xSampling != 0)
        {
            THROW (
                ArgExc,
                "Cannot shift image horizontally by "
                    << dx
                    << " "
                       "pixels.  The shift distance must be a multiple "
                       "of the x sampling rate of all channels, but the "
                       "x sampling rate channel "
                    << i->first
                    << " "
                       "is "
                    << i->second.xSampling << ".");
        }

        if (dy % i->second.ySampling != 0)
        {
            THROW (
                ArgExc,
                "Cannot shift image vertically by "
                    << dy
                    << " "
                       "pixels.  The shift distance must be a multiple "
                       "of the y sampling rate of all channels, but the "
                       "y sampling rate channel "
                    << i->first
                    << " "
                       "is "
                    << i->second.ySampling << ".");
        }
    }

    _dataWindow.min.x += dx;
    _dataWindow.min.y += dy;
    _dataWindow.max.x += dx;
    _dataWindow.max.y += dy;

    for (int y = 0; y < _levels.height (); ++y)
        for (int x = 0; x < _levels.width (); ++x)
            if (_levels[y][x]) _levels[y][x]->shiftPixels (dx, dy);
}

void
Image::insertChannel (
    const std::string& name,
    PixelType          type,
    int                xSampling,
    int                ySampling,
    bool               pLinear)
{
    try
    {
        _channels[name] = ChannelInfo (type, xSampling, ySampling, pLinear);

        for (int y = 0; y < _levels.height (); ++y)
            for (int x = 0; x < _levels.width (); ++x)
                if (_levels[y][x])
                    _levels[y][x]->insertChannel (
                        name, type, xSampling, ySampling, pLinear);
    }
    catch (...)
    {
        eraseChannel (name);
        throw;
    }
}

void
Image::insertChannel (const string& name, const Channel& channel)
{
    insertChannel (
        name,
        channel.type,
        channel.xSampling,
        channel.ySampling,
        channel.pLinear);
}

void
Image::eraseChannel (const std::string& name)
{
    //
    // Note: eraseChannel() is called to clean up if an exception is
    // thrown during a call during insertChannel(), so eraseChannel()
    // must work correctly even after an incomplete insertChannel()
    // operation.
    //

    for (int y = 0; y < _levels.height (); ++y)
        for (int x = 0; x < _levels.width (); ++x)
            if (_levels[y][x]) _levels[y][x]->eraseChannel (name);

    ChannelMap::iterator i = _channels.find (name);

    if (i != _channels.end ()) _channels.erase (i);
}

void
Image::clearChannels ()
{
    for (int y = 0; y < _levels.height (); ++y)
        for (int x = 0; x < _levels.width (); ++x)
            if (_levels[y][x]) _levels[y][x]->clearChannels ();

    _channels.clear ();
}

void
Image::renameChannel (const string& oldName, const string& newName)
{
    if (oldName == newName) return;

    ChannelMap::iterator oldChannel = _channels.find (oldName);

    if (oldChannel == _channels.end ())
    {
        THROW (
            ArgExc,
            "Cannot rename image channel " << oldName
                                           << " "
                                              "to "
                                           << newName
                                           << ".  The image does not have "
                                              "a channel called "
                                           << oldName << ".");
    }

    if (_channels.find (newName) != _channels.end ())
    {
        THROW (
            ArgExc,
            "Cannot rename image channel " << oldName
                                           << " "
                                              "to "
                                           << newName
                                           << ".  The image already has "
                                              "a channel called "
                                           << newName << ".");
    }

    try
    {
        for (int y = 0; y < _levels.height (); ++y)
            for (int x = 0; x < _levels.width (); ++x)
                if (_levels[y][x])
                    _levels[y][x]->renameChannel (oldName, newName);

        _channels[newName] = oldChannel->second;
        _channels.erase (oldChannel);
    }
    catch (...)
    {
        eraseChannel (oldName);
        eraseChannel (newName);
        throw;
    }
}

void
Image::renameChannels (const RenamingMap& oldToNewNames)
{
    set<string> newNames;

    for (ChannelMap::const_iterator i = _channels.begin ();
         i != _channels.end ();
         ++i)
    {
        RenamingMap::const_iterator j = oldToNewNames.find (i->first);
        std::string newName           = (j == oldToNewNames.end ()) ? i->first
                                                                    : j->second;

        if (newNames.find (newName) != newNames.end ())
        {
            THROW (
                ArgExc,
                "Cannot rename image channels.  More than one "
                "channel would be named \""
                    << newName << "\".");
        }
        else { newNames.insert (newName); }
    }

    try
    {
        renameChannelsInMap (oldToNewNames, _channels);

        for (int y = 0; y < _levels.height (); ++y)
            for (int x = 0; x < _levels.width (); ++x)
                if (_levels[y][x])
                    _levels[y][x]->renameChannels (oldToNewNames);
    }
    catch (...)
    {
        clearChannels ();
        throw;
    }
}

ImageLevel&
Image::level (int l)
{
    return level (l, l);
}

const ImageLevel&
Image::level (int l) const
{
    return level (l, l);
}

ImageLevel&
Image::level (int lx, int ly)
{
    if (!levelNumberIsValid (lx, ly))
    {
        THROW (
            ArgExc,
            "Cannot access image level with invalid "
            "level number ("
                << lx << ", " << ly << ").");
    }

    return *_levels[ly][lx];
}

const ImageLevel&
Image::level (int lx, int ly) const
{
    if (!levelNumberIsValid (lx, ly))
    {
        THROW (
            ArgExc,
            "Cannot access image level with invalid "
            "level number ("
                << lx << ", " << ly << ").");
    }

    return *_levels[ly][lx];
}

bool
Image::levelNumberIsValid (int lx, int ly) const
{
    return lx >= 0 && lx < _levels.width () && ly >= 0 &&
           ly < _levels.height () && _levels[ly][lx] != 0;
}

void
Image::clearLevels ()
{
    _dataWindow = Box2i (V2i (0, 0), V2i (-1, -1));

    for (int y = 0; y < _levels.height (); ++y)
        for (int x = 0; x < _levels.width (); ++x)
            delete _levels[y][x];

    _levels.resizeErase (0, 0);
}

Image::ChannelInfo::ChannelInfo (
    PixelType type, int xSampling, int ySampling, bool pLinear)
    : type (type)
    , xSampling (xSampling)
    , ySampling (ySampling)
    , pLinear (pLinear)
{
    // empty
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
