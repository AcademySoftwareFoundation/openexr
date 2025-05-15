//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class InputFile
//
//-----------------------------------------------------------------------------

#include "ImfInputFile.h"

#include "ImfCheckedArithmetic.h"

#include "ImfChannelList.h"
#include "ImfInputPartData.h"
#include "ImfInputStreamMutex.h"
#include "ImfMisc.h"
#include "ImfMultiPartInputFile.h"
#include "ImfPartType.h"
#include "ImfScanLineInputFile.h"
#include "ImfStdIO.h"
#include "ImfTiledInputFile.h"
#include "ImfVersion.h"

#include "ImfCompositeDeepScanLine.h"
#include "ImfDeepScanLineInputFile.h"

#include "Iex.h"
#include <ImathFun.h>
#include <half.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

struct InputFile::Data
{
    static constexpr int kDefaultPart = 0;

    Data (Context* ctxt, int t) : _ctxt (ctxt), _numThreads (t) {}
    ~Data ()
    {
        deleteCachedBuffer ();
    }

    const Header& getHeader (int part)
    {
        int pc = _ctxt->partCount ();
#if ILMTHREAD_THREADING_ENABLED
        std::lock_guard<std::mutex> lk (_mx);
#endif
        if (part >= 0 && part < pc)
        {
            int cursz = static_cast<int> (_lazy_header_cache.size ());
            // lazily build up the cache of headers
            if (part >= cursz)
            {
                _lazy_header_cache.resize (static_cast<size_t> (part) + 1);
                for (int i = cursz; i <= part; ++i)
                    _lazy_header_cache[i] = _ctxt->header (i);
            }
            return _lazy_header_cache[part];
        }
        else
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Invalid out of bounds part number " << part << ", only " << pc
                                                     << " parts in "
                                                     << _ctxt->fileName ());
        }
    }

    void setFrameBuffer (const FrameBuffer& frameBuffer);
    void lockedSetFrameBuffer (const FrameBuffer& frameBuffer);

    void readPixels (int scanline1, int scanline2);
    void bufferedReadPixels (int scanline1, int scanline2);

    void readPixels (
        const FrameBuffer& frameBuffer, int scanline1, int scanline2);

    void deleteCachedBuffer (void);
    void copyCachedBuffer (FrameBuffer::ConstIterator to,
                           FrameBuffer::ConstIterator from,
                           int scanline1, int scanline2,
                           int yStart, int xStart, int width);
    void fillBuffer (FrameBuffer::ConstIterator to,
                     int scanline1, int scanline2,
                     int yStart, int xStart, int width);

    int getPartIdx () const { return _part ? _part->partNumber : kDefaultPart; }
#if ILMTHREAD_THREADING_ENABLED
    // TODO: remove once we can do everything init in the constructor
    // and just use the C interface
    std::mutex _mx;
#endif
    Context*      _ctxt;
    int           _numThreads = 0;
    exr_storage_t _storage;

    // TODO: remove
    InputPartData*                         _part = nullptr;
    std::unique_ptr<MultiPartInputFile>    _mFile;
    std::unique_ptr<TiledInputFile>        _tFile;
    std::unique_ptr<ScanLineInputFile>     _sFile;
    std::unique_ptr<DeepScanLineInputFile> _dsFile;
    std::unique_ptr<CompositeDeepScanLine>
        _compositor; // for loading deep files

    // TODO: remove once we can remove deprecated API or to make it slow so
    // people switch to the new UI
    std::vector<Header> _lazy_header_cache;

    FrameBuffer                  _cacheFrameBuffer;
    int                          _cachedTileY  = -1;
    int                          _cachedOffset = 0;
    std::unique_ptr<FrameBuffer> _cachedBuffer;
    std::vector< std::unique_ptr<char[]> > _slicePointers;
};

InputFile::InputFile (
    const char* filename, const ContextInitializer& ctxtinit, int numThreads)
    : _data (std::make_shared<Data> (&_ctxt, numThreads))
{
    _data->_mFile.reset (new MultiPartInputFile (filename, ctxtinit, numThreads, false));
    _data->_part = _data->_mFile->getPart (Data::kDefaultPart);
    _ctxt = _data->_part->context;

    initialize ();
}

InputFile::InputFile (const char filename[], int numThreads)
    : InputFile (filename,
                 ContextInitializer ()
                 .silentHeaderParse (true)
                 .strictHeaderValidation (false),
                 numThreads)
{}

InputFile::InputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int numThreads)
    : InputFile (
          is.fileName (),
          ContextInitializer ()
          .silentHeaderParse (true)
          .strictHeaderValidation (false)
          .setInputStream (&is),
          numThreads)
{}

InputFile::InputFile (InputPartData* part)
    : _ctxt (part->context),
      _data (std::make_shared<Data> (&_ctxt, part->numThreads))
{
    _data->_part = part;
    initialize ();
}

const char*
InputFile::fileName () const
{
    return _ctxt.fileName ();
}

const Header&
InputFile::header () const
{
    if (_data->_part) return _data->_part->header;
    return _data->getHeader (Data::kDefaultPart);
}

int
InputFile::version () const
{
    if (_data->_part) return _data->_part->context.version ();
    return _ctxt.version ();
}

void
InputFile::setFrameBuffer (const FrameBuffer& frameBuffer)
{
    _data->setFrameBuffer (frameBuffer);
}

const FrameBuffer&
InputFile::frameBuffer () const
{
    // Really not actual protection once this returns...
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif

    if (_data->_compositor) { return _data->_compositor->frameBuffer (); }

    return _data->_cacheFrameBuffer;
}

bool
InputFile::isComplete () const
{
    return _ctxt.chunkTableValid (_data->getPartIdx ());
}

bool
InputFile::isOptimizationEnabled () const
{
    // TODO: the core library has a number of special cased patterns,
    // this is all kind of ... not useful? for now, return a pattern
    // similar to legacy version
    return _ctxt.channels (_data->getPartIdx ())->num_channels != 2;
}

void
InputFile::readPixels (int scanLine1, int scanLine2)
{
    _data->readPixels (scanLine1, scanLine2);
}

void
InputFile::readPixels (int scanLine)
{
    _data->readPixels (scanLine, scanLine);
}

void
InputFile::readPixels (
    const FrameBuffer& frameBuffer, int scanLine1, int scanLine2)
{
    _data->readPixels (frameBuffer, scanLine1, scanLine2);
}

void
InputFile::rawPixelData (
    int firstScanLine, const char*& pixelData, int& pixelDataSize)
{
    _data->_sFile->rawPixelData (firstScanLine, pixelData, pixelDataSize);
}

void
InputFile::rawPixelDataToBuffer (
    int scanLine, char* pixelData, int& pixelDataSize) const
{
    _data->_sFile->rawPixelDataToBuffer (scanLine, pixelData, pixelDataSize);
}

void
InputFile::rawTileData (
    int&         dx,
    int&         dy,
    int&         lx,
    int&         ly,
    const char*& pixelData,
    int&         pixelDataSize)
{
    try
    {
        if (_data->_storage != EXR_STORAGE_TILED &&
            _data->_storage != EXR_STORAGE_DEEP_TILED)
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Tried to read a raw tile "
                "from a scanline-based image.");
        }

        _data->_tFile->rawTileData (dx, dy, lx, ly, pixelData, pixelDataSize);
    }
    catch (IEX_NAMESPACE::BaseExc& e)
    {
        REPLACE_EXC (
            e,
            "Error reading tile data from image "
            "file \""
                << fileName () << "\". " << e.what ());
        throw;
    }
}

TiledInputFile&
InputFile::asTiledInput (void) const
{
    return *(_data->_tFile);
}

void
InputFile::initialize (void)
{
    int partidx;

    partidx = _data->getPartIdx ();
    _data->_storage = _ctxt.storage (partidx);

    // silly protection rules mean make_unique can't be used here
    if (_data->_storage == EXR_STORAGE_DEEP_SCANLINE)
    {
        _data->_dsFile.reset (new DeepScanLineInputFile (_data->_part));
        _data->_compositor = std::make_unique<CompositeDeepScanLine> ();
        _data->_compositor->addSource (_data->_dsFile.get ());
    }
    else if (
        _data->_storage == EXR_STORAGE_DEEP_TILED ||
        _data->_storage == EXR_STORAGE_TILED)
    {
        _data->_tFile.reset (new TiledInputFile (_data->_part));
    }
    else if (_data->_storage == EXR_STORAGE_SCANLINE)
    {
        _data->_sFile.reset (new ScanLineInputFile (_data->_part));
    }
    else
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to handle data storage type in file '" << fileName ()
                                                           << "'");
    }
}

void
InputFile::Data::setFrameBuffer (const FrameBuffer& frameBuffer)
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lk (_mx);
#endif
    lockedSetFrameBuffer (frameBuffer);
}

void
InputFile::Data::lockedSetFrameBuffer (const FrameBuffer& frameBuffer)
{
    if (_storage == EXR_STORAGE_TILED)
    {
        //
        // We must invalidate the cached buffer if the new frame
        // buffer has a different set of channels than the old
        // frame buffer, or if the type of a channel has changed.
        //

        const FrameBuffer& oldFrameBuffer = _cacheFrameBuffer;

        FrameBuffer::ConstIterator i = oldFrameBuffer.begin ();
        FrameBuffer::ConstIterator j = frameBuffer.begin ();

        while (i != oldFrameBuffer.end () && j != frameBuffer.end ())
        {
            if (strcmp (i.name (), j.name ()) ||
                i.slice ().type != j.slice ().type)
                break;

            ++i;
            ++j;
        }

        if (i != oldFrameBuffer.end () || j != frameBuffer.end ())
        {
            //
            // Invalidate the cached buffer.
            //
            deleteCachedBuffer ();

            //
            // Create new a cached frame buffer.  It can hold a single
            // row of tiles.  The cached buffer can be reused for each
            // row of tiles because we set the yTileCoords parameter of
            // each Slice to true.
            //

            _cachedBuffer               = std::make_unique<FrameBuffer> ();
            int              partidx    = getPartIdx ();
            exr_attr_box2i_t dataWindow = _ctxt->dataWindow (partidx);
            _cachedOffset               = dataWindow.min.x;

            uint64_t tileRowSize =
                static_cast<uint64_t> (_tFile->tileYSize ()) *
                static_cast<uint64_t> (
                    static_cast<int64_t> (dataWindow.max.x) -
                    static_cast<int64_t> (dataWindow.min.x) +
                    1LL );

            // before we allocate a (potentially large) chunk of ram, let's
            // quick ensure we can read the tiles
            if (!_ctxt->chunkTableValid (getPartIdx ()))
            {
                THROW (
                    IEX_NAMESPACE::ArgExc,
                    "Unable to use generic API to read with (partially?) corrupt chunk table in "
                    << _ctxt->fileName () << ", part " << getPartIdx () );
            }

            for (FrameBuffer::ConstIterator k = frameBuffer.begin ();
                 k != frameBuffer.end ();
                 ++k)
            {
                Slice s = k.slice ();

                //
                // omit adding channels that are not listed - 'fill' channels are added later
                //
                if (_ctxt->hasChannel (partidx, k.name ()))
                {
                    uint64_t bytes = (s.type == OPENEXR_IMF_INTERNAL_NAMESPACE::HALF) ? 2 : 4;
                    uint64_t offset = bytes * _cachedOffset;
                    uint64_t tilebytes = bytes * tileRowSize;

                    _slicePointers.emplace_back (std::make_unique<char[]> (tilebytes));

                    _cachedBuffer->insert (
                        k.name (),
                        Slice (
                            s.type,
                            _slicePointers.back ().get() - offset,
                            bytes,
                            bytes * _tFile->levelWidth (0),
                            1,
                            1,
                            s.fillValue,
                            false,
                            true));
                }
            }
        }

        _cacheFrameBuffer = frameBuffer;
    }
    else if (
        _storage == EXR_STORAGE_DEEP_SCANLINE ||
        _storage == EXR_STORAGE_DEEP_TILED)
    {
        if (!_compositor)
            _compositor = std::make_unique<CompositeDeepScanLine> ();
        _compositor->setFrameBuffer (frameBuffer);
    }
    else
    {
        _sFile->setFrameBuffer (frameBuffer);
        _cacheFrameBuffer = frameBuffer;
    }
}

void
InputFile::Data::readPixels (int scanLine1, int scanLine2)
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_mx);
#endif

    if (_compositor) { _compositor->readPixels (scanLine1, scanLine2); }
    else if (_storage == EXR_STORAGE_TILED)
    {
        bufferedReadPixels (scanLine1, scanLine2);
    }
    else { _sFile->readPixels (scanLine1, scanLine2); }
}

void
InputFile::Data::readPixels (
    const FrameBuffer& frameBuffer, int scanLine1, int scanLine2)
{
    if (_compositor)
    {
#if ILMTHREAD_THREADING_ENABLED
        std::lock_guard<std::mutex> lock (_mx);
#endif
        _compositor->setFrameBuffer (frameBuffer);
        _compositor->readPixels (scanLine1, scanLine2);
    }
    else if (_storage == EXR_STORAGE_TILED)
    {
#if ILMTHREAD_THREADING_ENABLED
        std::lock_guard<std::mutex> lock (_mx);
#endif

        lockedSetFrameBuffer (frameBuffer);
        bufferedReadPixels (scanLine1, scanLine2);
    }
    else { _sFile->readPixels (frameBuffer, scanLine1, scanLine2); }
}

void
InputFile::Data::bufferedReadPixels (int scanLine1, int scanLine2)
{
    exr_attr_box2i_t dataWindow = _ctxt->dataWindow (getPartIdx ());

    //
    // bufferedReadPixels reads each row of tiles that intersect the
    // scan-line range (scanLine1 to scanLine2). The previous row of
    // tiles is cached in order to prevent redundant tile reads when
    // accessing scanlines sequentially.
    //

    int minY = std::min (scanLine1, scanLine2);
    int maxY = std::max (scanLine1, scanLine2);

    if (minY < dataWindow.min.y || maxY > dataWindow.max.y)
    {
        throw IEX_NAMESPACE::ArgExc ("Tried to read scan line outside "
                                     "the image file's data window.");
    }

    int minDy = (minY - dataWindow.min.y) / _tFile->tileYSize ();
    int maxDy = (maxY - dataWindow.min.y) / _tFile->tileYSize ();

    // if we're reading the whole thing, just skip the double copy entirely
    if (minY == dataWindow.min.y && maxY == dataWindow.max.y)
    {
        _tFile->setFrameBuffer (_cacheFrameBuffer);
        _tFile->readTiles (0, _tFile->numXTiles (0) - 1, minDy, maxDy, 0, 0);
        return;
    }

    _tFile->setFrameBuffer (*_cachedBuffer);

    for ( int j = minDy; j <= maxDy; ++j )
    {
        if (j != _cachedTileY)
        {
            //
            // We don't have any valid buffered info, so we need to read in
            // from the file.
            // if no channels are being read that are present in file, cachedBuffer will be empty
            //

            if (_cachedBuffer &&
                _cachedBuffer->begin () != _cachedBuffer->end ())
            {
                _tFile->readTiles (0, _tFile->numXTiles (0) - 1, j, j, 0, 0);
            }

            _cachedTileY = j;
        }

        IMATH_NAMESPACE::Box2i tileRange = _tFile->dataWindowForTile (0, j, 0);

        int minYThisRow = std::max (minY, tileRange.min.y);
        int maxYThisRow = std::min (maxY, tileRange.max.y);

        int xStart = dataWindow.min.x;
        int width = dataWindow.max.x - dataWindow.min.x + 1;

        for (FrameBuffer::ConstIterator k = _cacheFrameBuffer.begin ();
             k != _cacheFrameBuffer.end ();
             ++k)
        {
            if (_cachedBuffer)
            {
                FrameBuffer::ConstIterator c = _cachedBuffer->find (k.name ());
                if (c != _cachedBuffer->end ())
                {
                    copyCachedBuffer (k, c, minYThisRow, maxYThisRow,
                                      tileRange.min.y, xStart, width);
                    continue;
                }
            }
            fillBuffer (k, minYThisRow, maxYThisRow,
                        dataWindow.min.y, xStart, width);
        }
    }
}

////////////////////////////////////////

void
InputFile::Data::copyCachedBuffer (FrameBuffer::ConstIterator to,
                                   FrameBuffer::ConstIterator from,
                                   int scanline1, int scanline2,
                                   int yStart, int xStart, int width)
{
    Slice toSlice = to.slice ();
    if (toSlice.xSampling != 1 || toSlice.ySampling != 1)
        throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");
    Slice fromSlice = from.slice ();
    if (fromSlice.xSampling != 1 || fromSlice.ySampling != 1)
        throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");
    if (fromSlice.xTileCoords || !fromSlice.yTileCoords)
        throw IEX_NAMESPACE::ArgExc ("Invalid expectation around tile coords flags from setFrameBuffer.");

    if (toSlice.type != fromSlice.type)
        throw IEX_NAMESPACE::ArgExc ("Invalid type mismatch in slice from setFrameBuffer.");
    if (fromSlice.xStride != 2 && fromSlice.xStride != 4)
        throw IEX_NAMESPACE::ArgExc ("Unhandled type in copying tile cache slice.");

    for (int y = scanline1; y <= scanline2; ++y)
    {
        char* toPtr = toSlice.base;
        const char* fromPtr = fromSlice.base;

        if (toSlice.yTileCoords)
            toPtr += ( int64_t (y) - int64_t (yStart) ) * toSlice.yStride;
        else
            toPtr += int64_t (y) * toSlice.yStride;
        if (!toSlice.xTileCoords)
            toPtr += int64_t (xStart) * toSlice.xStride;

        fromPtr += ( int64_t (y) - int64_t (yStart) ) * fromSlice.yStride;
        if (fromSlice.xStride == 2)
        {
            fromPtr += int64_t (xStart) * 2;
            for (int x = 0; x < width; ++x)
            {
                *reinterpret_cast<uint16_t*> (toPtr) =
                    *reinterpret_cast<const uint16_t*> (fromPtr);
                toPtr += toSlice.xStride;
                fromPtr += 2;
            }
        }
        else
        {
            fromPtr += int64_t (xStart) * 4;
            for (int x = 0; x < width; ++x)
            {
                *reinterpret_cast<uint32_t*> (toPtr) =
                    *reinterpret_cast<const uint32_t*> (fromPtr);
                toPtr += toSlice.xStride;
                fromPtr += 4;
            }
        }
    }
}

////////////////////////////////////////

void
InputFile::Data::fillBuffer (FrameBuffer::ConstIterator to,
                             int scanline1, int scanline2,
                             int yStart, int xStart, int width)
{
    Slice toSlice = to.slice ();
    if (toSlice.xSampling != 1 || toSlice.ySampling != 1)
        throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");

    for (int y = scanline1; y <= scanline2; ++y)
    {
        char* toPtr = toSlice.base;

        if (toSlice.yTileCoords)
            toPtr += ( int64_t (y) - int64_t (yStart) ) * toSlice.yStride;
        else
            toPtr += int64_t (y) * toSlice.yStride;
        if (!toSlice.xTileCoords)
            toPtr += int64_t (xStart) * toSlice.xStride;

        //
        // Copy all pixels for the scanline in this row of tiles
        //

        switch (toSlice.type)
        {
            case UINT:
            {
                unsigned int fill =
                    static_cast<unsigned int> (toSlice.fillValue);
                for (int x = 0; x < width; ++x)
                {
                    *reinterpret_cast<unsigned int*> (toPtr) = fill;
                    toPtr += toSlice.xStride;
                }
                break;
            }
            case HALF:
            {
                half fill = toSlice.fillValue;
                for (int x = 0; x < width; ++x)
                {
                    *reinterpret_cast<half*> (toPtr) = fill;
                    toPtr += toSlice.xStride;
                }
                break;
            }
            case FLOAT:
            {
                float fill = toSlice.fillValue;
                for (int x = 0; x < width; ++x)
                {
                    *reinterpret_cast<float*> (toPtr) = fill;
                    toPtr += toSlice.xStride;
                }
                break;
            }
            case NUM_PIXELTYPES:
                break;
        }
    }
}

void
InputFile::Data::deleteCachedBuffer (void)
{
    _cachedBuffer.reset ();
    _slicePointers.clear ();
    _cachedTileY = -1;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
