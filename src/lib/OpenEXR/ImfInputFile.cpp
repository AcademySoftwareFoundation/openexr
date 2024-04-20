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
        if (part < pc)
        {
            int cursz = static_cast<int> (_lazy_header_cache.size ());
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
                Iex::ArgExc,
                "Invalid out of bounds part number " << part << ", only " << pc
                                                     << " parts in "
                                                     << _ctxt->fileName ());
        }
    }

    void setFrameBuffer (const FrameBuffer& frameBuffer);

    void readPixels (int scanline1, int scanline2);
    void bufferedReadPixels (int scanline1, int scanline2);

    void deleteCachedBuffer (void);

    IStream* getCompatStream () { return _ctxt->legacyIStream (getPartIdx ()); }

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

    // TODO: remove once we can remove deprecated API
    std::vector<char> _pixel_data_scratch;

    // TODO: remove once we can remove deprecated API or to make it slow so
    // people switch to the new UI
    std::vector<Header> _lazy_header_cache;

    FrameBuffer                  _cacheFrameBuffer;
    int                          _cachedTileY  = -1;
    int                          _cachedOffset = 0;
    std::unique_ptr<FrameBuffer> _cachedBuffer;
};

InputFile::InputFile (
    const char* filename, const ContextInitializer& ctxtinit, int numThreads)
    : _data (std::make_shared<Data> (&_ctxt, numThreads))
{
    _ctxt.startRead (filename, ctxtinit);
    initialize ();
}

InputFile::InputFile (const char filename[], int numThreads)
    : InputFile (filename, ContextInitializer (), numThreads)
{}

InputFile::InputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int numThreads)
    : InputFile (
          is.fileName (),
          ContextInitializer ().setInputStream (&is),
          numThreads)
{}

InputFile::InputFile (InputPartData* part)
    : _data (std::make_shared<Data> (&_ctxt, part->numThreads))
{
    // TODO: janky, this will reread things for now (should share
    // context between objects eventually)
    _ctxt.startRead (
        part->mutex->is->fileName (),
        ContextInitializer ().setInputStream (part->mutex->is));

    _data->_part = part;
    initialize ();
}

InputFile::~InputFile ()
{
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
    if (_data->_part) return _data->_part->version;
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
InputFile::rawPixelData (
    int firstScanLine, const char*& pixelData, int& pixelDataSize)
{
    uint64_t maxsize = 0;
    if (EXR_ERR_SUCCESS !=
        exr_get_chunk_unpacked_size (_ctxt, _data->getPartIdx (), &maxsize))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Unable to query data size of chunk in file '" << fileName ()
                                                           << "'");
    }

    // again, doesn't actually provide any safety given we're handing
    // back a pointer... but will at least prevent two threads
    // allocating at the same time and getting sliced
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    _data->_pixel_data_scratch.resize (maxsize);

    pixelData     = _data->_pixel_data_scratch.data ();
    pixelDataSize = static_cast<int> (maxsize);

    rawPixelDataToBuffer (
        firstScanLine, _data->_pixel_data_scratch.data (), pixelDataSize);
}

void
InputFile::rawPixelDataToBuffer (
    int scanLine, char* pixelData, int& pixelDataSize) const
{
    exr_chunk_info_t cinfo;
    if (EXR_ERR_SUCCESS == exr_read_scanline_chunk_info (
                               _ctxt, _data->getPartIdx (), scanLine, &cinfo))
    {
        if (cinfo.packed_size > static_cast<uint64_t> (pixelDataSize))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                    << fileName ()
                    << "\". Provided buffer is too small to read raw pixel data:"
                    << pixelDataSize << " bytes.");
        }

        pixelDataSize = static_cast<int> (cinfo.packed_size);

        if (EXR_ERR_SUCCESS !=
            exr_read_chunk (_ctxt, _data->getPartIdx (), &cinfo, pixelData))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                    << fileName () << "\". Unable to read raw pixel data of "
                    << pixelDataSize << " bytes.");
        }
    }
    else
    {
        if (_data->_storage == EXR_STORAGE_TILED)
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                    << fileName ()
                    << "\". Tried to read a raw scanline from a tiled image.");
        }
        else
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                    << fileName ()
                    << "\". Unable to query data block information.");
        }
    }
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
    if (_data->_part)
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Tried to initialize a copy tile input file "
            "from a multi-part-converted input file.");
    }

    return *(_data->_tFile);
}

void
InputFile::initialize (void)
{
    int partidx;

    if (!_data->_part && _ctxt.partCount() > 1)
    {
        IStream *s = _data->getCompatStream ();

        s->seekg (0);
        _data->_mFile.reset (new MultiPartInputFile (*s, _data->_numThreads));
        _data->_part = _data->_mFile->getPart (0);
    }

    partidx = _data->getPartIdx ();
    _data->_storage = _ctxt.storage (partidx);

    // silly protection rules mean make_unique can't be used here
    if (_data->_storage == EXR_STORAGE_DEEP_SCANLINE)
    {
        if (_data->_part)
        {
            _data->_dsFile.reset (new DeepScanLineInputFile (_data->_part));
        }
        else
        {
            _data->_dsFile.reset (new DeepScanLineInputFile (
                _data->getHeader (partidx),
                _data->getCompatStream (),
                _ctxt.version (),
                _data->_numThreads));
        }
        _data->_compositor = std::make_unique<CompositeDeepScanLine> ();
        _data->_compositor->addSource (_data->_dsFile.get ());
    }
    else if (
        _data->_storage == EXR_STORAGE_DEEP_TILED ||
        _data->_storage == EXR_STORAGE_TILED)
    {
        if (_data->_part)
        {
            _data->_tFile.reset (new TiledInputFile (_data->_part));
        }
        else
        {
            _data->_tFile.reset (new TiledInputFile (
                _data->getHeader (partidx),
                _data->getCompatStream (),
                _ctxt.version (),
                _data->_numThreads));
        }
    }
    else if (_data->_storage == EXR_STORAGE_SCANLINE)
    {
        if (_data->_part)
        {
            _data->_sFile.reset (new ScanLineInputFile (_data->_part));
        }
        else
        {
            _data->_sFile.reset (new ScanLineInputFile (
                _data->getHeader (partidx),
                _data->getCompatStream (),
                _data->_numThreads));
        }
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
                uint64_t (_tFile->tileYSize ()) *
                (static_cast<uint64_t> (dataWindow.max.x - dataWindow.min.x) +
                 1U);

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
                    switch (s.type)
                    {
                        case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
                            _cachedBuffer->insert (
                                k.name (),
                                Slice (
                                    UINT,
                                    (char*) (new unsigned int[tileRowSize] -
                                             _cachedOffset),
                                    sizeof (unsigned int),
                                    sizeof (unsigned int) *
                                        _tFile->levelWidth (0),
                                    1,
                                    1,
                                    s.fillValue,
                                    false,
                                    true));
                            break;

                        case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                            _cachedBuffer->insert (
                                k.name (),
                                Slice (
                                    HALF,
                                    (char*) (new half[tileRowSize] -
                                             _cachedOffset),
                                    sizeof (half),
                                    sizeof (half) * _tFile->levelWidth (0),
                                    1,
                                    1,
                                    s.fillValue,
                                    false,
                                    true));
                            break;

                        case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                            _cachedBuffer->insert (
                                k.name (),
                                Slice (
                                    OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
                                    (char*) (new float[tileRowSize] -
                                             _cachedOffset),
                                    sizeof (float),
                                    sizeof (float) * _tFile->levelWidth (0),
                                    1,
                                    1,
                                    s.fillValue,
                                    false,
                                    true));
                            break;

                        default:

                            throw IEX_NAMESPACE::ArgExc (
                                "Unknown pixel data type.");
                    }
                }
            }
            _tFile->setFrameBuffer (*_cachedBuffer);
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
InputFile::Data::bufferedReadPixels (int scanLine1, int scanLine2)
{
    exr_attr_box2i_t dataWindow = _ctxt->dataWindow (getPartIdx ());

    using IMATH_NAMESPACE::Box2i;
    using IMATH_NAMESPACE::divp;
    using IMATH_NAMESPACE::modp;

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

    //
    // The minimum and maximum y tile coordinates that intersect this
    // scanline range
    //

    int minDy = (minY - dataWindow.min.y) / _tFile->tileYSize ();
    int maxDy = (maxY - dataWindow.min.y) / _tFile->tileYSize ();

    //
    // Figure out which one is first in the file so we can read without seeking
    //

    int yStart, yEnd, yStep;

    if (_ctxt->lineOrder (getPartIdx ()) == EXR_LINEORDER_DECREASING_Y)
    {
        yStart = maxDy;
        yEnd   = minDy - 1;
        yStep  = -1;
    }
    else
    {
        yStart = minDy;
        yEnd   = maxDy + 1;
        yStep  = 1;
    }

    //
    // the number of pixels in a row of tiles
    //

    Box2i levelRange = _tFile->dataWindowForLevel (0);

    //
    // Read the tiles into our temporary framebuffer and copy them into
    // the user's buffer
    //

    for (int j = yStart; j != yEnd; j += yStep)
    {
        Box2i tileRange = _tFile->dataWindowForTile (0, j, 0);

        int minYThisRow = std::max (minY, tileRange.min.y);
        int maxYThisRow = std::min (maxY, tileRange.max.y);

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
                _tFile->readTiles (0, _tFile->numXTiles (0) - 1, j, j);
            }

            _cachedTileY = j;
        }

        //
        // Copy the data from our cached framebuffer into the user's
        // framebuffer.
        //

        for (FrameBuffer::ConstIterator k = _cacheFrameBuffer.begin ();
             k != _cacheFrameBuffer.end ();
             ++k)
        {

            Slice toSlice = k.slice (); // slice to read from
            char* toPtr;

            int xStart = levelRange.min.x;
            int yStart = minYThisRow;

            while (modp (xStart, toSlice.xSampling) != 0)
                ++xStart;

            while (modp (yStart, toSlice.ySampling) != 0)
                ++yStart;

            FrameBuffer::ConstIterator c = _cachedBuffer->find (k.name ());
            intptr_t toBase = reinterpret_cast<intptr_t> (toSlice.base);

            if (c != _cachedBuffer->end ())
            {
                //
                // output channel was read from source image: copy to output slice
                //
                Slice    fromSlice = c.slice (); // slice to write to
                intptr_t fromBase = reinterpret_cast<intptr_t> (fromSlice.base);

                int   size = pixelTypeSize (toSlice.type);
                char* fromPtr;

                for (int y = yStart; y <= maxYThisRow; y += toSlice.ySampling)
                {
                    //
                    // Set the pointers to the start of the y scanline in
                    // this row of tiles
                    //

                    fromPtr = reinterpret_cast<char*> (
                        fromBase + (y - tileRange.min.y) * fromSlice.yStride +
                        xStart * fromSlice.xStride);

                    toPtr = reinterpret_cast<char*> (
                        toBase + divp (y, toSlice.ySampling) * toSlice.yStride +
                        divp (xStart, toSlice.xSampling) * toSlice.xStride);

                    //
                    // Copy all pixels for the scanline in this row of tiles
                    //

                    for (int x = xStart; x <= levelRange.max.x;
                         x += toSlice.xSampling)
                    {
                        for (int i = 0; i < size; ++i)
                            toPtr[i] = fromPtr[i];

                        fromPtr += fromSlice.xStride * toSlice.xSampling;
                        toPtr += toSlice.xStride;
                    }
                }
            }
            else
            {

                //
                // channel wasn't present in source file: fill output slice
                //
                for (int y = yStart; y <= maxYThisRow; y += toSlice.ySampling)
                {

                    toPtr = reinterpret_cast<char*> (
                        toBase + divp (y, toSlice.ySampling) * toSlice.yStride +
                        divp (xStart, toSlice.xSampling) * toSlice.xStride);

                    //
                    // Copy all pixels for the scanline in this row of tiles
                    //

                    switch (toSlice.type)
                    {
                        case UINT: {
                            unsigned int fill =
                                static_cast<unsigned int> (toSlice.fillValue);
                            for (int x = xStart; x <= levelRange.max.x;
                                 x += toSlice.xSampling)
                            {
                                *reinterpret_cast<unsigned int*> (toPtr) = fill;
                                toPtr += toSlice.xStride;
                            }
                            break;
                        }
                        case HALF: {
                            half fill = toSlice.fillValue;
                            for (int x = xStart; x <= levelRange.max.x;
                                 x += toSlice.xSampling)
                            {
                                *reinterpret_cast<half*> (toPtr) = fill;
                                toPtr += toSlice.xStride;
                            }
                            break;
                        }
                        case FLOAT: {
                            float fill = toSlice.fillValue;
                            for (int x = xStart; x <= levelRange.max.x;
                                 x += toSlice.xSampling)
                            {
                                *reinterpret_cast<float*> (toPtr) = fill;
                                toPtr += toSlice.xStride;
                            }
                            break;
                        }
                        case NUM_PIXELTYPES: {
                            break;
                        }
                    }
                }
            }
        }
    }
}

void
InputFile::Data::deleteCachedBuffer (void)
{
    if (_cachedBuffer)
    {
        for (FrameBuffer::Iterator k = _cachedBuffer->begin ();
             k != _cachedBuffer->end ();
             ++k)
        {
            Slice& s = k.slice ();

            switch (s.type)
            {
                case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:

                    delete[] (((unsigned int*) s.base) + _cachedOffset);
                    break;

                case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:

                    delete[] ((half*) s.base + _cachedOffset);
                    break;

                case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:

                    delete[] (((float*) s.base) + _cachedOffset);
                    break;
                case NUM_PIXELTYPES:
                    throw (IEX_NAMESPACE::ArgExc ("Invalid pixel type"));
            }
        }

        _cachedBuffer.reset ();
    }
    _cachedTileY = -1;
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
