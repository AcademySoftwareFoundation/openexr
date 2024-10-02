//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	class TiledInputFile
//
//-----------------------------------------------------------------------------

#include "ImfTiledInputFile.h"

#include "Iex.h"

#include "IlmThreadPool.h"
#if ILMTHREAD_THREADING_ENABLED
#    include "IlmThreadProcessGroup.h"
#    include <mutex>
#endif

#include "ImfFrameBuffer.h"
#include "ImfInputPartData.h"

// TODO: remove once TiledOutput is converted
#include "ImfTileOffsets.h"
#include "ImfTiledMisc.h"

#include <algorithm>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace {

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
        const FrameBuffer *outfb,
        const std::vector<Slice> &filllist);

    void update_pointers (
        const FrameBuffer *outfb,
        int fb_absX, int fb_absY,
        int t_absX, int t_absY);

    void run_fill (
        const FrameBuffer *outfb,
        int fb_absX, int fb_absY,
        int t_absX, int t_absY,
        const std::vector<Slice> &filllist);

    bool                  first = true;
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

struct TiledInputFile::Data
{
    Data (Context *ctxt, int pN, int nT)
    : _ctxt (ctxt)
    , partNumber (pN)
    , numThreads (nT)
    {}

    void initialize ()
    {
        if (_ctxt->storage (partNumber) != EXR_STORAGE_TILED)
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

    void readTiles (int dx1, int dx2, int dy1, int dy2, int lx, int ly);

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

    // TODO: remove once we can remove deprecated API
    std::vector<char> _tile_data_scratch;

    FrameBuffer frameBuffer;
    std::vector<Slice> fill_list;

    std::vector<std::string> _failures;

#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;

    class TileBufferTask final : public ILMTHREAD_NAMESPACE::Task
    {
    public:
        TileBufferTask (
            ILMTHREAD_NAMESPACE::TaskGroup* group,
            Data*                   ifd,
            TileProcessGroup*       tileg,
            const FrameBuffer*      outfb,
            const exr_chunk_info_t& cinfo)
            : Task (group)
            , _outfb (outfb)
            , _ifd (ifd)
            , _tile (tileg->pop ())
            , _tile_group (tileg)
        {
            _tile->cinfo = cinfo;
        }

        ~TileBufferTask () override
        {
            _tile_group->push (_tile);
        }

        void execute () override;

    private:
        void run_decode ();

        const FrameBuffer* _outfb;
        Data*              _ifd;

        TileProcess*       _tile;
        TileProcessGroup*  _tile_group;
    };
#endif
};

TiledInputFile::TiledInputFile (
    const char*               filename,
    const ContextInitializer& ctxtinit,
    int                       numThreads)
    : _ctxt (filename, ctxtinit, Context::read_mode_t{})
    , _data (std::make_shared<Data> (&_ctxt, 0, numThreads))
{
    _data->initialize ();
}

TiledInputFile::TiledInputFile (const char fileName[], int numThreads)
    : TiledInputFile (
        fileName,
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false),
        numThreads)
{
}

TiledInputFile::TiledInputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int numThreads)
    : TiledInputFile (
        is.fileName (),
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .setInputStream (&is),
        numThreads)
{
}

TiledInputFile::TiledInputFile (InputPartData* part)
    : _ctxt (part->context),
      _data (std::make_shared<Data> (&_ctxt, part->partNumber, part->numThreads))
{
    _data->initialize ();
}

const char*
TiledInputFile::fileName () const
{
    return _ctxt.fileName ();
}

const Header&
TiledInputFile::header () const
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
TiledInputFile::version () const
{
    return _ctxt.version ();
}

void
TiledInputFile::setFrameBuffer (const FrameBuffer& frameBuffer)
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    _data->fill_list.clear ();

    for (FrameBuffer::ConstIterator j = frameBuffer.begin ();
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
}

const FrameBuffer&
TiledInputFile::frameBuffer () const
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    return _data->frameBuffer;
}

bool
TiledInputFile::isComplete () const
{
    return _ctxt.chunkTableValid (_data->partNumber);
}

void
TiledInputFile::readTiles (int dx1, int dx2, int dy1, int dy2, int lx, int ly)
{
    //
    // Read a range of tiles from the file into the framebuffer
    //

    try
    {
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

        _data->readTiles (dx1, dx2, dy1, dy2, lx, ly);
    }
    catch (IEX_NAMESPACE::BaseExc& e)
    {
        REPLACE_EXC (
            e,
            "Error reading pixel data from image "
            "file \""
                << fileName () << "\". " << e.what ());
        throw;
    }
}

void
TiledInputFile::readTiles (int dx1, int dx2, int dy1, int dy2, int l)
{
    readTiles (dx1, dx2, dy1, dy2, l, l);
}

void
TiledInputFile::readTile (int dx, int dy, int lx, int ly)
{
    readTiles (dx, dx, dy, dy, lx, ly);
}

void
TiledInputFile::readTile (int dx, int dy, int l)
{
    readTile (dx, dy, l, l);
}

void
TiledInputFile::rawTileData (
    int&         dx,
    int&         dy,
    int&         lx,
    int&         ly,
    const char*& pixelData,
    int&         pixelDataSize)
{
    exr_chunk_info_t cinfo;
    if (EXR_ERR_SUCCESS == exr_read_tile_chunk_info (
            _ctxt, _data->partNumber, dx, dy, lx, ly, &cinfo))
    {
#if ILMTHREAD_THREADING_ENABLED
        std::lock_guard<std::mutex> lock (_data->_mx);
#endif
        _data->_tile_data_scratch.resize (cinfo.packed_size);
        pixelDataSize = static_cast<int> (cinfo.packed_size);
        if (EXR_ERR_SUCCESS !=
            exr_read_chunk (_ctxt, _data->partNumber, &cinfo,
                            _data->_tile_data_scratch.data ()))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                    << fileName () << "\". Unable to read raw tile data of "
                    << pixelDataSize << " bytes.");
        }
        pixelData = _data->_tile_data_scratch.data ();
        dx = cinfo.start_x;
        dy = cinfo.start_y;
        lx = cinfo.level_x;
        ly = cinfo.level_y;
    }
    else
    {
        if (!isValidTile (dx, dy, lx, ly))
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading pixel data from image "
                "file \""
                << fileName () << "\". "
                << "Tried to read a tile outside "
                "the image file's data window.");
        }
        else
        {
            THROW (
                IEX_NAMESPACE::ArgExc,
                "Error reading chunk information for tile from image "
                "file \""
                << fileName () << "\". Unable to read raw tile offset information.");
        }
    }
}

unsigned int
TiledInputFile::tileXSize () const
{
    return _data->tile_x_size;
}

unsigned int
TiledInputFile::tileYSize () const
{
    return _data->tile_y_size;
}

LevelMode
TiledInputFile::levelMode () const
{
    return (LevelMode)_data->tile_level_mode;
}

LevelRoundingMode
TiledInputFile::levelRoundingMode () const
{
    return (LevelRoundingMode)_data->tile_round_mode;
}

int
TiledInputFile::numLevels () const
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
TiledInputFile::numXLevels () const
{
    return _data->num_x_levels;
}

int
TiledInputFile::numYLevels () const
{
    return _data->num_y_levels;
}

bool
TiledInputFile::isValidLevel (int lx, int ly) const
{
    if (lx < 0 || ly < 0) return false;

    if (levelMode () == MIPMAP_LEVELS && lx != ly) return false;

    if (lx >= numXLevels () || ly >= numYLevels ()) return false;

    return true;
}

int
TiledInputFile::levelWidth (int lx) const
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
TiledInputFile::levelHeight (int ly) const
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
TiledInputFile::numXTiles (int lx) const
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
TiledInputFile::numYTiles (int ly) const
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
TiledInputFile::dataWindowForLevel (int l) const
{
    return dataWindowForLevel (l, l);
}

IMATH_NAMESPACE::Box2i
TiledInputFile::dataWindowForLevel (int lx, int ly) const
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
TiledInputFile::dataWindowForTile (int dx, int dy, int l) const
{
    return dataWindowForTile (dx, dy, l, l);
}

IMATH_NAMESPACE::Box2i
TiledInputFile::dataWindowForTile (int dx, int dy, int lx, int ly) const
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
TiledInputFile::isValidTile (int dx, int dy, int lx, int ly) const
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
TiledInputFile::tileOrder (int dx[], int dy[], int lx[], int ly[]) const
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

void TiledInputFile::Data::readTiles (int dx1, int dx2, int dy1, int dy2, int lx, int ly)
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
                        new TileBufferTask (&tg, this, &tpg, &frameBuffer, cinfo) );
                }
            }
        }

        tpg.throw_on_failure ();
    }
    else
#endif
    {
        TileProcess tp;

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
void TiledInputFile::Data::TileBufferTask::execute ()
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
    const FrameBuffer *outfb,
    const std::vector<Slice> &filllist)
{
    int absX, absY, tileX, tileY;
    exr_attr_box2i_t dw;

    // stash the flag off to make sure to clean up in the event
    // of an exception by changing the flag after init...
    bool isfirst = first;
    if (first)
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_initialize (ctxt, pn, &cinfo, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to initialize decode pipeline");
        }

        first = false;
    }
    else
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_update (ctxt, pn, &cinfo, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to update decode pipeline");
        }
    }

    if (EXR_ERR_SUCCESS != exr_get_data_window (ctxt, pn, &dw))
        throw IEX_NAMESPACE::ArgExc ("Unable to query the data window.");

    if (EXR_ERR_SUCCESS != exr_get_tile_sizes (
            ctxt, pn, cinfo.level_x, cinfo.level_y, &tileX, &tileY))
        throw IEX_NAMESPACE::ArgExc ("Unable to query the data window.");

    absX = dw.min.x + tileX * cinfo.start_x;
    absY = dw.min.y + tileY * cinfo.start_y;

    update_pointers (outfb, dw.min.x, dw.min.y, absX, absY);

    if (isfirst)
    {
        if (EXR_ERR_SUCCESS !=
            exr_decoding_choose_default_routines (ctxt, pn, &decoder))
        {
            throw IEX_NAMESPACE::IoExc ("Unable to choose decoder routines");
        }
    }

    if (EXR_ERR_SUCCESS != exr_decoding_run (ctxt, pn, &decoder))
        throw IEX_NAMESPACE::IoExc ("Unable to run decoder");

    run_fill (outfb, dw.min.x, dw.min.y, absX, absY, filllist);
}

////////////////////////////////////////

void TileProcess::update_pointers (const FrameBuffer *outfb, int fb_absX, int fb_absY, int t_absX, int t_absY)
{
    decoder.user_line_begin_skip = 0;
    decoder.user_line_end_ignore = 0;

    for (int c = 0; c < decoder.channel_count; ++c)
    {
        exr_coding_channel_info_t& curchan = decoder.channels[c];
        uint8_t*                   ptr;
        const Slice*               fbslice;

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

        curchan.user_bytes_per_element = (fbslice->type == HALF) ? 2 : 4;
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
    const FrameBuffer *outfb, int fb_absX, int fb_absY, int t_absX, int t_absY,
    const std::vector<Slice> &filllist)
{
    for (auto& s: filllist)
    {
        uint8_t* ptr;

        if (s.xSampling != 1 || s.ySampling != 1)
            throw IEX_NAMESPACE::ArgExc ("Tiled data should not have subsampling.");

        int xOffset = s.xTileCoords ? 0 : t_absX;
        int yOffset = s.yTileCoords ? 0 : t_absY;

        ptr  = reinterpret_cast<uint8_t*> (s.base);
        ptr += int64_t (xOffset) * int64_t (s.xStride);
        ptr += int64_t (yOffset) * int64_t (s.yStride);

        // TODO: update ImfMisc, lift fill type / value
        for ( int start = 0; start < cinfo.height; ++start )
        {
            if (start % s.ySampling) continue;

            uint8_t* outptr = ptr;
            for ( int sx = 0; sx < cinfo.width; ++sx )
            {
                if (sx % s.xSampling) continue;

                switch (s.type)
                {
                    case OPENEXR_IMF_INTERNAL_NAMESPACE::UINT:
                    {
                        unsigned int fillVal = (unsigned int) (s.fillValue);
                        *(unsigned int*)outptr = fillVal;
                        break;
                    }

                    case OPENEXR_IMF_INTERNAL_NAMESPACE::HALF:
                    {
                        half fillVal = half (s.fillValue);
                        *(half*)outptr = fillVal;
                        break;
                    }

                    case OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT:
                    {
                        float fillVal = float (s.fillValue);
                        *(float*)outptr = fillVal;
                        break;
                    }
                    default:
                        throw IEX_NAMESPACE::ArgExc ("Unknown pixel data type.");
                }
                outptr += s.xStride;
            }

            ptr += s.yStride;
        }
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
