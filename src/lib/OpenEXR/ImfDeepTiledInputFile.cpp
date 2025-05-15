//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//      class DeepTiledInputFile
//
//-----------------------------------------------------------------------------

#include "ImfDeepTiledInputFile.h"

#include "Iex.h"

#include "IlmThreadPool.h"
#if ILMTHREAD_THREADING_ENABLED
#    include "IlmThreadProcessGroup.h"
#    include <mutex>
#endif

#include "ImfDeepFrameBuffer.h"
#include "ImfInputPartData.h"

// TODO: remove once TiledOutput is converted
#include "ImfTileOffsets.h"
#include "ImfTiledMisc.h"

#include <algorithm>
#include <string>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace {

// TODO: similarities to tiled input file...
struct TileProcess
{
    ~TileProcess ()
    {
        if (!first)
            exr_decoding_destroy (decoder.context, &decoder);
    }

    void run_decode (
        exr_const_context_t ctxt,
        int pn,
        const DeepFrameBuffer *outfb,
        const std::vector<DeepSlice> &filllist);

    void update_pointers (
        const DeepFrameBuffer *outfb,
        int fb_absX, int fb_absY,
        int t_absX, int t_absY);

    void run_fill (
        const DeepFrameBuffer *outfb,
        int fb_absX, int fb_absY,
        int t_absX, int t_absY,
        const std::vector<DeepSlice> &filllist);

    void copy_sample_count (
        const DeepFrameBuffer *outfb,
        int fb_absX, int fb_absY,
        int t_absX, int t_absY);

    exr_result_t          last_decode_err = EXR_ERR_UNKNOWN;
    bool                  first = true;
    bool                  counts_only = false;
    exr_chunk_info_t      cinfo;
    exr_decode_pipeline_t decoder;

    TileProcess*          next;
};

#if ILMTHREAD_THREADING_ENABLED
using TileProcessGroup = ILMTHREAD_NAMESPACE::ProcessGroup<TileProcess>;
#endif

} // empty namespace

//
// struct TiledInputFile::Data stores things that will be
// needed between calls to readTile()
//

struct DeepTiledInputFile::Data
{
    Data (Context *ctxt, int pN, int nT)
    : _ctxt (ctxt)
    , partNumber (pN)
    , numThreads (nT)
    {}

    void initialize ()
    {
        if (_ctxt->storage (partNumber) != EXR_STORAGE_DEEP_TILED)
            throw IEX_NAMESPACE::ArgExc ("File part is not a tiled part");

        if (EXR_ERR_SUCCESS != exr_get_tile_descriptor (
                *_ctxt,
                partNumber,
                &tile_x_size,
                &tile_y_size,
                &tile_level_mode,
                &tile_round_mode))
            throw IEX_NAMESPACE::ArgExc ("Unable to query tile descriptor");

        if (EXR_ERR_SUCCESS != exr_get_tile_levels (
                *_ctxt,
                partNumber,
                &num_x_levels,
                &num_y_levels))
            throw IEX_NAMESPACE::ArgExc ("Unable to query number of tile levels");
    }

    // TODO: generalize to have async framebuffer path
    void readTiles (int dx1, int dx2, int dy1, int dy2, int lx, int ly, bool countsOnly);

    Context* _ctxt;
    int partNumber;
    int numThreads;
    Header header;
    bool header_filled = false;

    uint32_t tile_x_size = 0;
    uint32_t tile_y_size = 0;
    exr_tile_level_mode_t tile_level_mode = EXR_TILE_LAST_TYPE;
    exr_tile_round_mode_t tile_round_mode = EXR_TILE_ROUND_LAST_TYPE;

    int32_t num_x_levels = 0;
    int32_t num_y_levels = 0;

    bool frameBufferValid = false;
    DeepFrameBuffer frameBuffer;
    std::vector<DeepSlice> fill_list;

#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;

    class TileBufferTask final : public ILMTHREAD_NAMESPACE::Task
    {
    public:
        TileBufferTask (
            ILMTHREAD_NAMESPACE::TaskGroup* group,
            Data*                   ifd,
            TileProcessGroup*       tileg,
            const DeepFrameBuffer*  outfb,
            const exr_chunk_info_t& cinfo,
            bool                    countsOnly)
            : Task (group)
            , _outfb (outfb)
            , _ifd (ifd)
            , _tile (tileg->pop ())
            , _tile_group (tileg)
        {
            _tile->cinfo = cinfo;
            _tile->counts_only = countsOnly;
        }

        ~TileBufferTask () override
        {
            _tile_group->push (_tile);
        }

        void execute () override;

    private:
        void run_decode ();

        const DeepFrameBuffer* _outfb;
        Data*                  _ifd;

        TileProcess*       _tile;
        TileProcessGroup*  _tile_group;
    };
#endif
};

DeepTiledInputFile::DeepTiledInputFile (
    const char*               filename,
    const ContextInitializer& ctxtinit,
    int                       numThreads)
    : _ctxt (filename, ctxtinit, Context::read_mode_t{})
    , _data (std::make_shared<Data> (&_ctxt, 0, numThreads))
{
    _data->initialize ();
}

DeepTiledInputFile::DeepTiledInputFile (const char fileName[], int numThreads)
    : DeepTiledInputFile (
        fileName,
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false),
        numThreads)
{
}

DeepTiledInputFile::DeepTiledInputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int numThreads)
    : DeepTiledInputFile (
        is.fileName (),
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .setInputStream (&is),
        numThreads)
{
}

DeepTiledInputFile::DeepTiledInputFile (InputPartData* part)
    : _ctxt (part->context),
      _data (std::make_shared<Data> (&_ctxt, part->partNumber, part->numThreads))
{
    _data->initialize ();
}

const char*
DeepTiledInputFile::fileName () const
{
    return _ctxt.fileName ();
}

const Header&
DeepTiledInputFile::header () const
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    if (!_data->header_filled)
    {
        _data->header = _ctxt.header (_data->partNumber);
        _data->header_filled = true;
    }
    return _data->header;
}

int
DeepTiledInputFile::version () const
{
    return _ctxt.version ();
}

void
DeepTiledInputFile::setFrameBuffer (const DeepFrameBuffer& frameBuffer)
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    _data->fill_list.clear ();

    for (DeepFrameBuffer::ConstIterator j = frameBuffer.begin ();
         j != frameBuffer.end ();
         ++j)
    {
        const exr_attr_chlist_entry_t* curc = _ctxt.findChannel (
            _data->partNumber, j.name ());

        if (!curc)
        {
            _data->fill_list.push_back (j.slice ());
            continue;
        }

        if (curc->x_sampling != j.slice ().xSampling ||
            curc->y_sampling != j.slice ().ySampling)
            THROW (
                IEX_NAMESPACE::ArgExc,
                "X and/or y subsampling factors "
                "of \""
                    << j.name ()
                    << "\" channel "
                       "of input file \""
                    << fileName ()
                    << "\" are "
                       "not compatible with the frame buffer's "
                       "subsampling factors.");
    }

    _data->frameBuffer = frameBuffer;
    _data->frameBufferValid = true;
}

const DeepFrameBuffer&
DeepTiledInputFile::frameBuffer () const
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    return _data->frameBuffer;
}

bool
DeepTiledInputFile::isComplete () const
{
    return _ctxt.chunkTableValid (_data->partNumber);
}

void
DeepTiledInputFile::readTiles (
    int dx1, int dx2, int dy1, int dy2, int lx, int ly)
{
    //
    // Read a range of tiles from the file into the framebuffer
    //

    try
    {
        if (!_data->frameBufferValid)
        {
            throw IEX_NAMESPACE::ArgExc (
                "readTiles called with no valid frame buffer");
        }

        if (!isValidLevel (lx, ly))
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Level coordinate "
                "(" << lx
                    << ", " << ly
                    << ") "
                       "is invalid.");

        if (dx1 > dx2) std::swap (dx1, dx2);
        if (dy1 > dy2) std::swap (dy1, dy2);

        _data->readTiles (dx1, dx2, dy1, dy2, lx, ly, false);
    }
    catch (IEX_NAMESPACE::BaseExc& e)
    {
        REPLACE_EXC (
            e,
            "Error reading deep tiled data from image "
            "file \""
                << fileName () << "\". " << e.what ());
        throw;
    }
}

void
DeepTiledInputFile::readTiles (int dx1, int dx2, int dy1, int dy2, int l)
{
    readTiles (dx1, dx2, dy1, dy2, l, l);
}

void
DeepTiledInputFile::readTile (int dx, int dy, int lx, int ly)
{
    readTiles (dx, dx, dy, dy, lx, ly);
}

void
DeepTiledInputFile::readTile (int dx, int dy, int l)
{
    readTile (dx, dy, l, l);
}

#pragma pack(push, 1)
struct DeepTileChunkHeader
{
    int32_t dx;
    int32_t dy;
    int32_t lx;
    int32_t ly;
    uint64_t packedCountSize;
    uint64_t packedDataSize;
    uint64_t unpackedDataSize;
};
#pragma pack(pop)

void
DeepTiledInputFile::rawTileData (
    int&      dx,
    int&      dy,
    int&      lx,
    int&      ly,
    char*     pixelData,
    uint64_t& pixelDataSize) const
{
    exr_chunk_info_t cinfo;

    static_assert (sizeof(DeepTileChunkHeader) == 40, "Expect a 40-byte tiled chunk header");

    if (EXR_ERR_SUCCESS == exr_read_tile_chunk_info (
            _ctxt, _data->partNumber, dx, dy, lx, ly, &cinfo))
    {
        uint64_t cbytes;
        cbytes = sizeof (DeepTileChunkHeader);
        cbytes += cinfo.sample_count_table_size;
        cbytes += cinfo.packed_size;

        if (!pixelData || cbytes > pixelDataSize)
        {
            pixelDataSize = cbytes;
            return;
        }

        pixelDataSize = cbytes;

        DeepTileChunkHeader* dch = reinterpret_cast<DeepTileChunkHeader*> (pixelData);
        dch->dx = cinfo.start_x;
        dch->dy = cinfo.start_y;
        dch->lx = cinfo.level_x;
        dch->ly = cinfo.level_y;
        dch->packedCountSize = cinfo.sample_count_table_size;
        dch->packedDataSize = cinfo.packed_size;
        dch->unpackedDataSize = cinfo.unpacked_size;

        pixelData += sizeof(DeepTileChunkHeader);

        if (EXR_ERR_SUCCESS !=
            exr_read_deep_chunk (
                _ctxt,
                _data->partNumber,
                &cinfo,
                pixelData + cinfo.sample_count_table_size,
                pixelData))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading deep tiled data from image "
                "file \""
                << fileName () << "\". Unable to read raw tile data of "
                << pixelDataSize << " bytes.");
        }
    }
    else
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error reading deep tile data from image "
            "file \""
            << fileName ()
            << "\". Unable to query data block information.");
    }
}

unsigned int
DeepTiledInputFile::tileXSize () const
{
    return _data->tile_x_size;
}

unsigned int
DeepTiledInputFile::tileYSize () const
{
    return _data->tile_y_size;
}

LevelMode
DeepTiledInputFile::levelMode () const
{
    return (LevelMode)_data->tile_level_mode;
}

LevelRoundingMode
DeepTiledInputFile::levelRoundingMode () const
{
    return (LevelRoundingMode)_data->tile_round_mode;
}

int
DeepTiledInputFile::numLevels () const
{
    if (levelMode () == RIPMAP_LEVELS)
        THROW (
            IEX_NAMESPACE::LogicExc,
            "Error calling numLevels() on image "
            "file \""
                << fileName ()
                << "\" "
                   "(numLevels() is not defined for files "
                   "with RIPMAP level mode).");

    return _data->num_x_levels;
}

int
DeepTiledInputFile::numXLevels () const
{
    return _data->num_x_levels;
}

int
DeepTiledInputFile::numYLevels () const
{
    return _data->num_y_levels;
}

bool
DeepTiledInputFile::isValidLevel (int lx, int ly) const
{
    if (lx < 0 || ly < 0) return false;

    if (levelMode () == MIPMAP_LEVELS && lx != ly) return false;

    if (lx >= numXLevels () || ly >= numYLevels ()) return false;

    return true;
}

int
DeepTiledInputFile::levelWidth (int lx) const
{
    int32_t levw = 0;
    if (EXR_ERR_SUCCESS != exr_get_level_sizes (
            _ctxt, _data->partNumber, lx, 0, &levw, nullptr))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error calling levelWidth() on image "
            "file \""
                << fileName () << "\".");
    }
    return levw;
}

int
DeepTiledInputFile::levelHeight (int ly) const
{
    int32_t levh = 0;
    if (EXR_ERR_SUCCESS != exr_get_level_sizes (
            _ctxt, _data->partNumber, 0, ly, nullptr, &levh))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error calling levelWidth() on image "
            "file \""
                << fileName () << "\".");
    }
    return levh;
}

int
DeepTiledInputFile::numXTiles (int lx) const
{
    int32_t countx = 0;
    if (EXR_ERR_SUCCESS != exr_get_tile_counts (
            _ctxt, _data->partNumber, lx, 0, &countx, nullptr))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error calling numXTiles() on image "
            "file \""
                << fileName () << "\".");
    }
    return countx;
}

int
DeepTiledInputFile::numYTiles (int ly) const
{
    int32_t county = 0;
    if (EXR_ERR_SUCCESS != exr_get_tile_counts (
            _ctxt, _data->partNumber, 0, ly, nullptr, &county))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error calling numYTiles() on image "
            "file \""
                << fileName () << "\".");
    }
    return county;
}

IMATH_NAMESPACE::Box2i
DeepTiledInputFile::dataWindowForLevel (int l) const
{
    return dataWindowForLevel (l, l);
}

IMATH_NAMESPACE::Box2i
DeepTiledInputFile::dataWindowForLevel (int lx, int ly) const
{
    int32_t levw = 0, levh = 0;
    if (EXR_ERR_SUCCESS != exr_get_level_sizes (
            _ctxt, _data->partNumber, lx, ly, &levw, &levh))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error calling dataWindowForLevel() on image "
            "file \""
                << fileName () << "\".");
    }
    exr_attr_box2i_t dw = _ctxt.dataWindow (_data->partNumber);
    return IMATH_NAMESPACE::Box2i (
        IMATH_NAMESPACE::V2i (dw.min.x, dw.min.y),
        IMATH_NAMESPACE::V2i (dw.min.x + levw - 1, dw.min.y + levh - 1));
}

IMATH_NAMESPACE::Box2i
DeepTiledInputFile::dataWindowForTile (int dx, int dy, int l) const
{
    return dataWindowForTile (dx, dy, l, l);
}

IMATH_NAMESPACE::Box2i
DeepTiledInputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
{
    try
    {
        if (!isValidTile (dx, dy, lx, ly))
            throw IEX_NAMESPACE::ArgExc ("Arguments not in valid range.");

        //exr_attr_box2i_t dw = _ctxt.dataWindow (_data->partNumber);
        auto dw = dataWindowForLevel (lx, ly);

        int32_t tileSizeX, tileSizeY;
        if (EXR_ERR_SUCCESS !=
            exr_get_tile_sizes (_ctxt, _data->partNumber, lx, ly, &tileSizeX, &tileSizeY))
          throw IEX_NAMESPACE::ArgExc ("Unable to query the data window.");

        dw.min.x += dx * tileSizeX;
        dw.min.y += dy * tileSizeY;
        int limX = dw.min.x + tileSizeX - 1;
        int limY = dw.min.y + tileSizeY - 1;
        limX = std::min (limX, dw.max.x);
        limY = std::min (limY, dw.max.y);

        return IMATH_NAMESPACE::Box2i (
            IMATH_NAMESPACE::V2i (dw.min.x, dw.min.y),
            IMATH_NAMESPACE::V2i (limX, limY));
    }
    catch (IEX_NAMESPACE::BaseExc& e)
    {
        REPLACE_EXC (
            e,
            "Error calling dataWindowForTile() on image "
            "file \""
                << fileName () << "\". " << e.what ());
        throw;
    }
}

bool
DeepTiledInputFile::isValidTile (int dx, int dy, int lx, int ly) const
{
    int32_t countx = 0, county = 0;
    if (EXR_ERR_SUCCESS == exr_get_tile_counts (
            _ctxt, _data->partNumber, lx, ly, &countx, &county))
    {
        // get tile counts will check lx, ly for us
        return ((dx < countx && dx >= 0) &&
                (dy < county && dy >= 0));
    }
    return false;
}

void
DeepTiledInputFile::readPixelSampleCounts (
    int dx1, int dx2, int dy1, int dy2, int lx, int ly)
{
    //
    // Read a range of tiles from the file into the framebuffer
    //
    try
    {
        if (!_data->frameBufferValid)
        {
            throw IEX_NAMESPACE::ArgExc (
                "readPixelSampleCounts called with no valid frame buffer");
        }

        if (!isValidLevel (lx, ly))
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Level coordinate "
                "(" << lx
                    << ", " << ly
                    << ") "
                       "is invalid.");

        if (dx1 > dx2) std::swap (dx1, dx2);
        if (dy1 > dy2) std::swap (dy1, dy2);

        _data->readTiles (dx1, dx2, dy1, dy2, lx, ly, true);
    }
    catch (IEX_NAMESPACE::BaseExc& e)
    {
        REPLACE_EXC (
            e,
            "Error reading deep pixel sample counts data from image "
            "file \""
                << fileName () << "\". " << e.what ());
        throw;
    }
}

void
DeepTiledInputFile::readPixelSampleCount (int dx, int dy, int l)
{
    readPixelSampleCount (dx, dy, l, l);
}

void
DeepTiledInputFile::readPixelSampleCount (int dx, int dy, int lx, int ly)
{
    readPixelSampleCounts (dx, dx, dy, dy, lx, ly);
}

void
DeepTiledInputFile::readPixelSampleCounts (
    int dx1, int dx2, int dy1, int dy2, int l)
{
    readPixelSampleCounts (dx1, dx2, dy1, dy2, l, l);
}

size_t
DeepTiledInputFile::totalTiles () const
{
    //
    // Calculate the total number of tiles in the file
    //

    int numAllTiles = 0;

    switch (levelMode ())
    {
        case ONE_LEVEL:
        case MIPMAP_LEVELS:

            for (int i_l = 0; i_l < numLevels (); ++i_l)
                numAllTiles += numXTiles (i_l) * numYTiles (i_l);

            break;

        case RIPMAP_LEVELS:

            for (int i_ly = 0; i_ly < numYLevels (); ++i_ly)
                for (int i_lx = 0; i_lx < numXLevels (); ++i_lx)
                    numAllTiles += numXTiles (i_lx) * numYTiles (i_ly);

            break;

        default: throw IEX_NAMESPACE::ArgExc ("Unknown LevelMode format.");
    }
    return numAllTiles;
}

namespace
{
struct tilepos
{
    uint64_t filePos;
    int      dx;
    int      dy;
    int      lx;
    int      ly;
    bool     operator< (const tilepos& other) const
    {
        return filePos < other.filePos;
    }
};
} // namespace

void
DeepTiledInputFile::getTileOrder (int dx[], int dy[], int lx[], int ly[]) const
{
    // TODO: remove once TiledOutputFile copy is converted
    switch (_ctxt.lineOrder (_data->partNumber))
    {
        case EXR_LINEORDER_RANDOM_Y:
            // calc below outside the nest
            break;

        case EXR_LINEORDER_DECREASING_Y:
        {
            dx[0] = 0;
            dy[0] = numYTiles (0) - 1;
            lx[0] = 0;
            ly[0] = 0;
            return;
        }
        case EXR_LINEORDER_INCREASING_Y:
            dx[0] = 0;
            dy[0] = 0;
            lx[0] = 0;
            ly[0] = 0;
            return;

        case EXR_LINEORDER_LAST_TYPE: /* invalid but should never be here */
        default:
            throw IEX_NAMESPACE::ArgExc ("Unknown LineOrder.");
    }

    size_t numAllTiles = 0;
    int numX = numXLevels ();
    int numY = numYLevels ();

    switch (levelMode ())
    {
        case ONE_LEVEL:
        case MIPMAP_LEVELS:
            for (int i_l = 0; i_l < numY; ++i_l)
                numAllTiles += size_t (numXTiles (i_l)) * size_t (numYTiles (i_l));
            break;

        case RIPMAP_LEVELS:
            for (int i_ly = 0; i_ly < numY; ++i_ly)
                for (int i_lx = 0; i_lx < numX; ++i_lx)
                    numAllTiles += size_t (numXTiles (i_lx)) * size_t (numYTiles (i_ly));
            break;

        default:
            throw IEX_NAMESPACE::ArgExc ("Unknown LevelMode format.");
    }

    std::vector<tilepos> table;

    table.resize (numAllTiles);
    size_t tIdx = 0;
    switch (levelMode ())
    {
        case ONE_LEVEL:
        case MIPMAP_LEVELS:
            for (int i_l = 0; i_l < numY; ++i_l)
            {
                int nY = numYTiles (i_l);
                int nX = numXTiles (i_l);

                for ( int y = 0; y < nY; ++y )
                    for ( int x = 0; x < nX; ++x )
                    {
                        exr_chunk_info_t cinfo;
                        if (EXR_ERR_SUCCESS == exr_read_tile_chunk_info (
                                _ctxt, _data->partNumber, x, y, i_l, i_l, &cinfo))
                        {
                            tilepos &tp = table[tIdx++];
                            tp.filePos = cinfo.data_offset;
                            tp.dx = x;
                            tp.dy = y;
                            tp.lx = i_l;
                            tp.ly = i_l;
                        }
                        else
                        {
                            throw IEX_NAMESPACE::ArgExc ("Unable to get tile offset.");
                        }
                    }
            }
            break;

        case RIPMAP_LEVELS:
            for (int i_ly = 0; i_ly < numY; ++i_ly)
            {
                int nY = numYTiles (i_ly);
                for (int i_lx = 0; i_lx < numX; ++i_lx)
                {
                    int nX = numXTiles (i_lx);
                    for ( int y = 0; y < nY; ++y )
                        for ( int x = 0; x < nX; ++x )
                        {
                            exr_chunk_info_t cinfo;
                            if (EXR_ERR_SUCCESS == exr_read_tile_chunk_info (
                                    _ctxt, _data->partNumber, x, y, i_lx, i_ly, &cinfo))
                            {
                                tilepos &tp = table[tIdx++];
                                tp.filePos = cinfo.data_offset;
                                tp.dx = x;
                                tp.dy = y;
                                tp.lx = i_lx;
                                tp.ly = i_ly;
                            }
                            else
                            {
                                throw IEX_NAMESPACE::ArgExc ("Unable to get tile offset.");
                            }
                        }
                }
            }
            break;

        default: throw IEX_NAMESPACE::ArgExc ("Unknown LevelMode format.");
    }

    std::sort (table.begin(), table.end ());

    for (size_t i = 0; i < numAllTiles; ++i)
    {
        const auto& tp = table[i];
        dx[i] = tp.dx;
        dy[i] = tp.dy;
        lx[i] = tp.lx;
        ly[i] = tp.ly;
    }
}

void DeepTiledInputFile::Data::readTiles (
    int dx1, int dx2, int dy1, int dy2, int lx, int ly, bool countsOnly)
{
    int nTiles = dx2 - dx1 + 1;
    nTiles *= dy2 - dy1 + 1;

    exr_chunk_info_t      cinfo;
#if ILMTHREAD_THREADING_ENABLED
    if (nTiles > 1 && numThreads > 1)
    {
        // we need the lifetime of this to last longer than the
        // lifetime of the task group below such that we don't get use
        // after free type error, so use scope rules to accomplish
        // this
        TileProcessGroup tpg (numThreads);

        {
            ILMTHREAD_NAMESPACE::TaskGroup tg;

            for (int ty = dy1; ty <= dy2; ++ty)
            {
                for (int tx = dx1; tx <= dx2; ++tx)
                {
                    exr_result_t rv = exr_read_tile_chunk_info (
                        *_ctxt, partNumber, tx, ty, lx, ly, &cinfo);
                    if (EXR_ERR_INCOMPLETE_CHUNK_TABLE == rv)
                    {
                        THROW (
                            IEX_NAMESPACE::InputExc,
                            "Tile (" << tx << ", " << ty << ", " << lx << ", " << ly
                            << ") is missing.");
                    }
                    else if (EXR_ERR_SUCCESS != rv)
                        throw IEX_NAMESPACE::InputExc ("Unable to query tile information");

                    ILMTHREAD_NAMESPACE::ThreadPool::addGlobalTask (
                        new TileBufferTask (&tg, this, &tpg, &frameBuffer, cinfo, countsOnly) );
                }
            }
        }

        tpg.throw_on_failure ();
    }
    else
#endif
    {
        TileProcess tp;

        tp.counts_only = countsOnly;
        for (int ty = dy1; ty <= dy2; ++ty)
        {
            for (int tx = dx1; tx <= dx2; ++tx)
            {
                exr_result_t rv = exr_read_tile_chunk_info (
                    *_ctxt, partNumber, tx, ty, lx, ly, &cinfo);
                if (EXR_ERR_INCOMPLETE_CHUNK_TABLE == rv)
                {
                    THROW (
                        IEX_NAMESPACE::InputExc,
                        "Tile (" << tx << ", " << ty << ", " << lx << ", " << ly
                        << ") is missing.");
                }
                else if (EXR_ERR_SUCCESS != rv)
                    throw IEX_NAMESPACE::InputExc ("Unable to query tile information");

                tp.cinfo = cinfo;
                tp.run_decode (
                    *_ctxt,
                    partNumber,
                    &frameBuffer,
                    fill_list);
            }
        }
    }
}

////////////////////////////////////////

#if ILMTHREAD_THREADING_ENABLED
void DeepTiledInputFile::Data::TileBufferTask::execute ()
{
    try
    {
        _tile->run_decode (
            *(_ifd->_ctxt),
            _ifd->partNumber,
            _outfb,
            _ifd->fill_list);
    }
    catch (std::exception &e)
    {
        _tile_group->record_failure (e.what ());
    }
    catch (...)
    {
        _tile_group->record_failure ("Unknown exception");
    }
}
#endif

////////////////////////////////////////

void TileProcess::run_decode (
    exr_const_context_t ctxt,
    int pn,
    const DeepFrameBuffer *outfb,
    const std::vector<DeepSlice> &filllist)
{
    int absX, absY, tileX, tileY;
    exr_attr_box2i_t dw;
    uint8_t flags;

    if (first)
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_initialize (ctxt, pn, &cinfo, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to initialize decode pipeline");
        }
        decoder.decode_flags |= EXR_DECODE_NON_IMAGE_DATA_AS_POINTERS;
        decoder.decode_flags |= EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL;
        flags = 0;

        first = false;
    }
    else
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_update (ctxt, pn, &cinfo, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to update decode pipeline");
        }
        flags = decoder.decode_flags;
    }

    if (EXR_ERR_SUCCESS != exr_get_data_window (ctxt, pn, &dw))
        throw IEX_NAMESPACE::ArgExc ("Unable to query the data window.");

    if (EXR_ERR_SUCCESS != exr_get_tile_sizes (
            ctxt, pn, cinfo.level_x, cinfo.level_y, &tileX, &tileY))
        throw IEX_NAMESPACE::ArgExc ("Unable to query the data window.");

    absX = dw.min.x + tileX * cinfo.start_x;
    absY = dw.min.y + tileY * cinfo.start_y;

    if (counts_only)
        decoder.decode_flags |= EXR_DECODE_SAMPLE_DATA_ONLY;
    else
        decoder.decode_flags = decoder.decode_flags & ~EXR_DECODE_SAMPLE_DATA_ONLY;

    update_pointers (outfb, dw.min.x, dw.min.y, absX, absY);

    if (flags != decoder.decode_flags)
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_choose_default_routines (ctxt, pn, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to choose decoder routines");
        }
    }

    last_decode_err = exr_decoding_run (ctxt, pn, &decoder);
    if (EXR_ERR_SUCCESS != last_decode_err)
    {
        THROW (
            IEX_NAMESPACE::IoExc,
            "Unable to run decoder: "
            << exr_get_error_code_as_string (last_decode_err));
    }

    copy_sample_count (outfb, dw.min.x, dw.min.y, absX, absY);

    if (counts_only)
        return;

    run_fill (outfb, dw.min.x, dw.min.y, absX, absY, filllist);
}

////////////////////////////////////////

void TileProcess::update_pointers (const DeepFrameBuffer *outfb, int fb_absX, int fb_absY, int t_absX, int t_absY)
{
    decoder.user_line_begin_skip = 0;
    decoder.user_line_end_ignore = 0;

    for (int c = 0; c < decoder.channel_count; ++c)
    {
        exr_coding_channel_info_t& curchan = decoder.channels[c];
        uint8_t*                   ptr;
        const DeepSlice*           fbslice;

        fbslice = outfb->findSlice (curchan.channel_name);

        if (curchan.height == 0 || !fbslice)
        {
            curchan.decode_to_ptr     = NULL;
            curchan.user_pixel_stride = 0;
            curchan.user_line_stride  = 0;
            continue;
        }

        if (fbslice->xSampling != 1 || fbslice->ySampling != 1)
            throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");

        int xOffset = fbslice->xTileCoords ? 0 : t_absX;
        int yOffset = fbslice->yTileCoords ? 0 : t_absY;

        curchan.user_bytes_per_element = fbslice->sampleStride;
        curchan.user_data_type         = (exr_pixel_type_t)fbslice->type;
        curchan.user_pixel_stride      = fbslice->xStride;
        curchan.user_line_stride       = fbslice->yStride;

        ptr  = reinterpret_cast<uint8_t*> (fbslice->base);
        ptr += int64_t (xOffset) * int64_t (fbslice->xStride);
        ptr += int64_t (yOffset) * int64_t (fbslice->yStride);

        curchan.decode_to_ptr = ptr;
    }
}

////////////////////////////////////////

void TileProcess::run_fill (
    const DeepFrameBuffer *outfb, int fb_absX, int fb_absY, int t_absX, int t_absY,
    const std::vector<DeepSlice> &filllist)
{
    for (auto& fills: filllist)
    {
        uint8_t* ptr;

        if (fills.xSampling != 1 || fills.ySampling != 1)
            throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");

        int xOffset = fills.xTileCoords ? 0 : t_absX;
        int yOffset = fills.yTileCoords ? 0 : t_absY;

        ptr  = reinterpret_cast<uint8_t*> (fills.base);
        ptr += int64_t (xOffset) * int64_t (fills.xStride);
        ptr += int64_t (yOffset) * int64_t (fills.yStride);

        // TODO: update ImfMisc, lift fill type / value
        for ( int start = 0; start < cinfo.height; ++start )
        {
            const int32_t* counts = decoder.sample_count_table;
            counts += (int64_t) start * (int64_t) cinfo.width;

            uint8_t* outptr = ptr;
            for ( int sx = 0; sx < cinfo.width; ++sx )
            {
                int32_t samps = counts[sx];
                void *dest = *((void **)outptr);

                if (samps == 0 || dest == nullptr)
                {
                    outptr += fills.xStride;
                    continue;
                }

                switch (fills.type)
                {
                    case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
                    {
                        unsigned int fillVal = (unsigned int) (fills.fillValue);
                        unsigned int* fillptr = static_cast<unsigned int*> (dest);

                        for ( int32_t s = 0; s < samps; ++s )
                            fillptr[s] = fillVal;
                        break;
                    }

                    case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:
                    {
                        half fillVal = half (fills.fillValue);
                        half* fillptr = static_cast<half*> (dest);

                        for ( int32_t s = 0; s < samps; ++s )
                            fillptr[s] = fillVal;
                        break;
                    }

                    case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:
                    {
                        float fillVal = float (fills.fillValue);
                        float* fillptr = static_cast<float*> (dest);

                        for ( int32_t s = 0; s < samps; ++s )
                            fillptr[s] = fillVal;
                        break;
                    }
                    default:
                        throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
                }
                outptr += fills.xStride;
            }

            ptr += fills.yStride;
        }
    }
}

////////////////////////////////////////

void TileProcess::copy_sample_count (
    const DeepFrameBuffer *outfb,
    int fb_absX, int fb_absY,
    int t_absX, int t_absY)
{
    uint8_t*     ptr;
    const Slice& s = outfb->getSampleCountSlice ();
    if (s.xSampling != 1 || s.ySampling != 1)
        throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");

    int xOffset = s.xTileCoords ? 0 : t_absX;
    int yOffset = s.yTileCoords ? 0 : t_absY;

    ptr  = reinterpret_cast<uint8_t*> (s.base);
    ptr += int64_t (xOffset) * int64_t (s.xStride);
    ptr += int64_t (yOffset) * int64_t (s.yStride);

    int64_t xS = int64_t (s.xStride);
    int64_t yS = int64_t (s.yStride);

    for ( int y = 0; y < cinfo.height; ++y )
    {
        const int32_t* counts = decoder.sample_count_table + y * cinfo.width;

        if (xS == sizeof(int32_t))
        {
            memcpy (ptr, counts, cinfo.width * sizeof(int32_t));
        }
        else
        {
            uint8_t* lineptr = ptr;
            for ( int x = 0; x < cinfo.width; ++x )
            {
                *((int32_t *)lineptr) = counts[x];
                lineptr += xS;
            }
        }
        ptr += yS;
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
