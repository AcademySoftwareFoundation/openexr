//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//      class DeepScanLineInputFile
//
//-----------------------------------------------------------------------------

#include "ImfDeepScanLineInputFile.h"

#include "ImfDeepFrameBuffer.h"
#include "ImfInputPartData.h"

#include "IlmThreadPool.h"
#if ILMTHREAD_THREADING_ENABLED
#    include "IlmThreadProcessGroup.h"
#    include <mutex>
#endif

#include "Iex.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace {

// same as normal scanline but w/ deep framebuffer
// TODO: template?
struct ScanLineProcess
{
    ~ScanLineProcess ()
    {
        if (!first)
            exr_decoding_destroy (decoder.context, &decoder);
    }

    void run_mem_decode (
        exr_const_context_t ctxt,
        int pn,
        const char *rawdata,
        const DeepFrameBuffer *outfb,
        int fbY,
        int fbLastY,
        const std::vector<DeepSlice> &filllist);

    void run_decode (
        exr_const_context_t ctxt,
        int pn,
        const DeepFrameBuffer *outfb,
        int fbY,
        int fbLastY,
        const std::vector<DeepSlice> &filllist);

    void run_unpack (
        exr_const_context_t ctxt,
        int pn,
        const DeepFrameBuffer *outfb,
        int fbY,
        int fbLastY,
        const std::vector<DeepSlice> &filllist);

    void update_pointers (
        const DeepFrameBuffer *outfb,
        int fbY,
        int fbLastY);

    void run_fill (
        const DeepFrameBuffer *outfb,
        int fbY,
        const std::vector<DeepSlice> &filllist);

    void copy_sample_count (
        const DeepFrameBuffer *outfb,
        int fbY);

    exr_result_t          last_decode_err = EXR_ERR_UNKNOWN;
    bool                  first = true;
    bool                  counts_only = false;
    exr_chunk_info_t      cinfo;
    exr_decode_pipeline_t decoder;

    ScanLineProcess*      next;
};

#if ILMTHREAD_THREADING_ENABLED
using ScanLineProcessGroup = ILMTHREAD_NAMESPACE::ProcessGroup<ScanLineProcess>;
#endif

} // empty namespace

struct DeepScanLineInputFile::Data
{
    Data (Context *ctxt, int pN, int nT)
    : _ctxt (ctxt)
    , partNumber (pN)
    , numThreads (nT)
    {}

    void initialize ()
    {
        if (_ctxt->storage (partNumber) != EXR_STORAGE_DEEP_SCANLINE)
            throw IEX_NAMESPACE::ArgExc ("File part is not a deep scanline part");

        version = _ctxt->version ();
    }

    std::pair<int, int> getChunkRange (int y) const;

    void readData (const DeepFrameBuffer &fb, int scanLine1, int scanLine2, bool countsOnly);
    void readMemData (
        const DeepFrameBuffer &fb,
        const char *rawPixelData,
        int scanLine1,
        int scanLine2,
        bool countsOnly);

    void prepFillList (const DeepFrameBuffer &fb, std::vector<DeepSlice> &fill);

    Context* _ctxt;
    int partNumber;
    int numThreads;
    int version;
    Header header;
    bool header_filled = false;

    bool frameBufferValid = false;
    DeepFrameBuffer frameBuffer;
    std::vector<DeepSlice> fill_list;

#if ILMTHREAD_THREADING_ENABLED
    std::mutex _mx;

    class LineBufferTask final : public ILMTHREAD_NAMESPACE::Task
    {
    public:
        LineBufferTask (
            ILMTHREAD_NAMESPACE::TaskGroup* group,
            Data*                   ifd,
            ScanLineProcessGroup*   lineg,
            const DeepFrameBuffer*  outfb,
            const exr_chunk_info_t& cinfo,
            int                     fby,
            int                     endScan,
            bool                    countsOnly)
            : Task (group)
            , _outfb (outfb)
            , _ifd (ifd)
            , _fby (fby)
            , _last_fby (endScan)
            , _line (lineg->pop ())
            , _line_group (lineg)
        {
            _line->cinfo = cinfo;
            _line->counts_only = countsOnly;
        }

        ~LineBufferTask () override
        {
            _line_group->push (_line);
        }

        void execute () override;

    private:
        void run_decode ();

        const DeepFrameBuffer* _outfb;
        Data*                  _ifd;
        int                    _fby;
        int                    _last_fby;
        ScanLineProcess*       _line;
        ScanLineProcessGroup*  _line_group;
    };
#endif
};

DeepScanLineInputFile::DeepScanLineInputFile (InputPartData* part)
    : _ctxt (part->context),
      _data (std::make_shared<Data> (&_ctxt, part->partNumber, part->numThreads))
{
    _data->initialize ();
}

DeepScanLineInputFile::DeepScanLineInputFile (
    const char*               filename,
    const ContextInitializer& ctxtinit,
    int                       numThreads)
    : _ctxt (filename, ctxtinit, Context::read_mode_t{})
    , _data (std::make_shared<Data> (&_ctxt, 0, numThreads))
{
    _data->initialize ();
}

DeepScanLineInputFile::DeepScanLineInputFile (
    const char fileName[], int numThreads)
    : DeepScanLineInputFile (
        fileName,
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false),
        numThreads)
{
}

DeepScanLineInputFile::DeepScanLineInputFile (
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream& is, int numThreads)
    : DeepScanLineInputFile (
        is.fileName (),
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .setInputStream (&is),
        numThreads)
{
}

DeepScanLineInputFile::DeepScanLineInputFile (
    const Header&                            header,
    OPENEXR_IMF_INTERNAL_NAMESPACE::IStream* is,
    int                                      version,
    int                                      numThreads)
    : DeepScanLineInputFile (
        is->fileName (),
        ContextInitializer ()
        .silentHeaderParse (true)
        .strictHeaderValidation (false)
        .setInputStream (is),
        numThreads)
{
    // who uses this interface, can we remove it?
    _data->version = version;
    _data->header = header;
    _data->header_filled = true;
}

const char*
DeepScanLineInputFile::fileName () const
{
    return _ctxt.fileName ();
}

const Header&
DeepScanLineInputFile::header () const
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
DeepScanLineInputFile::version () const
{
    return _data->version;
}

void
DeepScanLineInputFile::setFrameBuffer (const DeepFrameBuffer& frameBuffer)
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    _data->prepFillList (frameBuffer, _data->fill_list);
    _data->frameBuffer = frameBuffer;
    _data->frameBufferValid = true;
}

const DeepFrameBuffer&
DeepScanLineInputFile::frameBuffer () const
{
#if ILMTHREAD_THREADING_ENABLED
    std::lock_guard<std::mutex> lock (_data->_mx);
#endif
    return _data->frameBuffer;
}

bool
DeepScanLineInputFile::isComplete () const
{
    return _ctxt.chunkTableValid (_data->partNumber);
}

void
DeepScanLineInputFile::readPixels (int scanLine1, int scanLine2)
{
    if (!_data->frameBufferValid)
    {
        throw IEX_NAMESPACE::ArgExc (
            "readPixels called with no valid frame buffer");
    }

    _data->readData (_data->frameBuffer, scanLine1, scanLine2, false);
}

void
DeepScanLineInputFile::readPixels (int scanLine)
{
    readPixels (scanLine, scanLine);
}

#pragma pack(push, 1)
struct DeepChunkHeader
{
    int32_t scanline;
    uint64_t packedCountSize;
    uint64_t packedDataSize;
    uint64_t unpackedDataSize;
};
#pragma pack(pop)

void
DeepScanLineInputFile::rawPixelData (
    int firstScanLine, char* pixelData, uint64_t& pixelDataSize)
{
    exr_chunk_info_t cinfo;

    static_assert (sizeof(DeepChunkHeader) == 28, "Expect a 28-byte chunk header");

    // api is kind of different than the normal scanline raw pixel data
    // in that it also includes the chunk header block, so the full chunk

    if (EXR_ERR_SUCCESS == exr_read_scanline_chunk_info (
            _ctxt, _data->partNumber, firstScanLine, &cinfo))
    {
        uint64_t cbytes;
        cbytes = sizeof (DeepChunkHeader);
        cbytes += cinfo.sample_count_table_size;
        cbytes += cinfo.packed_size;

        if (!pixelData || cbytes > pixelDataSize)
        {
            pixelDataSize = cbytes;
            return;
        }

        pixelDataSize = cbytes;

        DeepChunkHeader* dch = reinterpret_cast<DeepChunkHeader*> (pixelData);
        dch->scanline = cinfo.start_y;
        dch->packedCountSize = cinfo.sample_count_table_size;
        dch->packedDataSize = cinfo.packed_size;
        dch->unpackedDataSize = cinfo.unpacked_size;

        pixelData += sizeof(DeepChunkHeader);
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
                "Error reading deep pixel data from image "
                "file \""
                << fileName () << "\". Unable to read raw pixel data of "
                << pixelDataSize << " bytes.");
        }
    }
    else
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error reading deep pixel data from image "
            "file \""
            << fileName ()
            << "\". Unable to query data block information.");
    }
}

void
DeepScanLineInputFile::readPixels (
    const char*            rawPixelData,
    const DeepFrameBuffer& frameBuffer,
    int                    scanLine1,
    int                    scanLine2) const
{
    _data->readMemData (frameBuffer, rawPixelData, scanLine1, scanLine2, false);
}

void
DeepScanLineInputFile::readPixelSampleCounts (
    const char*            rawPixelData,
    const DeepFrameBuffer& frameBuffer,
    int                    scanLine1,
    int                    scanLine2) const
{
    _data->readMemData (frameBuffer, rawPixelData, scanLine1, scanLine2, true);
}

void
DeepScanLineInputFile::readPixelSampleCounts (int scanline1, int scanline2)
{
    if (!_data->frameBufferValid)
    {
        throw IEX_NAMESPACE::ArgExc (
            "readPixelSampleCounts called with no valid frame buffer");
    }

    _data->readData (_data->frameBuffer, scanline1, scanline2, true);
}

void
DeepScanLineInputFile::readPixelSampleCounts (int scanline)
{
    readPixelSampleCounts (scanline, scanline);
}

int
DeepScanLineInputFile::firstScanLineInChunk (int y) const
{
    return _data->getChunkRange (y).first;
}

int
DeepScanLineInputFile::lastScanLineInChunk (int y) const
{
    return _data->getChunkRange (y).second;
}

std::pair<int, int> DeepScanLineInputFile::Data::getChunkRange (int y) const
{
    exr_attr_box2i_t dw = _ctxt->dataWindow (partNumber);
    int32_t scansperchunk = 1;

    if (y < dw.min.y || y > dw.max.y)
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Requested scanline " << y << " is outside "
            "the image file's data window: "
            << dw.min.y << " - " << dw.max.y);
    }

    if (EXR_ERR_SUCCESS != exr_get_scanlines_per_chunk (*_ctxt, partNumber, &scansperchunk))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error querying scanline counts from image "
            "file \"" << _ctxt->fileName () << "\".");
    }

    if (scansperchunk == 1)
        return std::make_pair( y, y );

    std::pair<int, int> ret;
    int64_t yoff;

    yoff = (int64_t) y;
    yoff -= (int64_t) dw.min.y;
    yoff /= (int64_t) scansperchunk;

    yoff *= (int64_t) scansperchunk;
    yoff += (int64_t) dw.min.y;

    ret.first = (int32_t) yoff;
    yoff += (int64_t) scansperchunk;
    yoff = std::min (yoff, (int64_t) dw.max.y);
    ret.second = (int32_t) yoff;

    return ret;
}

void
DeepScanLineInputFile::Data::readData (
    const DeepFrameBuffer &fb, int scanLine1, int scanLine2, bool countsOnly)
{
    exr_attr_box2i_t dw = _ctxt->dataWindow (partNumber);
    exr_chunk_info_t cinfo;
    int32_t          scansperchunk = 1;

    if (EXR_ERR_SUCCESS != exr_get_scanlines_per_chunk (*_ctxt, partNumber, &scansperchunk))
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Error querying scanline counts from image "
            "file \"" << _ctxt->fileName () << "\".");
    }

    if (scanLine2 < scanLine1)
        std::swap (scanLine1, scanLine2);

    if (scanLine1 < dw.min.y || scanLine2 > dw.max.y)
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "Tried to read scan line outside "
            "the image file's data window: "
            << scanLine1 << " - " << scanLine2
            << " vs datawindow "
            << dw.min.y << " - " << dw.max.y);
    }

#if ILMTHREAD_THREADING_ENABLED
    int64_t nchunks;
    nchunks = ((int64_t) scanLine2 - (int64_t) scanLine1);
    nchunks /= (int64_t) scansperchunk;
    nchunks += 1;

    if (nchunks > 1 && numThreads > 1)
    {
        // we need the lifetime of this to last longer than the
        // lifetime of the task group below such that we don't get use
        // after free type error, so use scope rules to accomplish
        // this
        ScanLineProcessGroup sg (numThreads);

        {
            ILMTHREAD_NAMESPACE::TaskGroup tg;

            for (int y = scanLine1; y <= scanLine2; )
            {
                if (EXR_ERR_SUCCESS != exr_read_scanline_chunk_info (*_ctxt, partNumber, y, &cinfo))
                    throw IEX_NAMESPACE::InputExc ("Unable to query scanline information");

                ILMTHREAD_NAMESPACE::ThreadPool::addGlobalTask (
                    new LineBufferTask (&tg, this, &sg, &fb, cinfo, y, scanLine2, countsOnly) );

                y += scansperchunk - (y - cinfo.start_y);
            }
        }

        sg.throw_on_failure ();
    }
    else
#endif
    {
        ScanLineProcess sp;
        bool redo = true;

        sp.counts_only = countsOnly;
        for (int y = scanLine1; y <= scanLine2; )
        {
            if (EXR_ERR_SUCCESS != exr_read_scanline_chunk_info (*_ctxt, partNumber, y, &cinfo))
                throw IEX_NAMESPACE::InputExc ("Unable to query scanline information");

            // Check if we have the same chunk where we can just
            // re-run the unpack (i.e. people reading 1 scan at a time
            // in a multi-scanline chunk)
            if (!redo &&
                sp.cinfo.idx == cinfo.idx &&
                sp.last_decode_err == EXR_ERR_SUCCESS)
            {
                sp.run_unpack (
                    *_ctxt,
                    partNumber,
                    &fb,
                    y,
                    scanLine2,
                    fill_list);
            }
            else
            {
                sp.cinfo = cinfo;
                sp.run_decode (
                    *_ctxt,
                    partNumber,
                    &fb,
                    y,
                    scanLine2,
                    fill_list);
                redo = false;
            }

            y += scansperchunk - (y - cinfo.start_y);
        }
    }
}

////////////////////////////////////////

void
DeepScanLineInputFile::Data::readMemData (
        const DeepFrameBuffer &fb,
        const char *rawPixelData,
        int scanLine1,
        int scanLine2,
        bool countsOnly)
{
    const DeepChunkHeader* dch = reinterpret_cast<const DeepChunkHeader*> (rawPixelData);
    std::pair<int, int> range = getChunkRange (scanLine1);

    if (dch->scanline != scanLine1)
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "readPixelSampleCounts(rawPixelData,frameBuffer,"
                << scanLine1 << ',' << scanLine2
                << ") called with incorrect start scanline - should be "
                << dch->scanline);
    }

    if (scanLine2 != range.second)
    {
        THROW (
            IEX_NAMESPACE::ArgExc,
            "readPixelSampleCounts(rawPixelData,frameBuffer,"
                << scanLine1 << ',' << scanLine2
                << ") called with incorrect end scanline - should be " << range.second);
    }

    rawPixelData += sizeof(DeepChunkHeader);

    std::vector<DeepSlice> fills;
    ScanLineProcess proc;

    if (!countsOnly)
        prepFillList(fb, fills);

    if (EXR_ERR_SUCCESS != exr_read_scanline_chunk_info (
            *_ctxt, partNumber, scanLine1, &proc.cinfo))
        throw IEX_NAMESPACE::InputExc ("Unable to query scanline information");

    proc.counts_only = countsOnly;
    proc.run_mem_decode (
        *_ctxt,
        partNumber,
        rawPixelData,
        &frameBuffer,
        scanLine1,
        scanLine2,
        fills);
}

////////////////////////////////////////

void DeepScanLineInputFile::Data::prepFillList (
    const DeepFrameBuffer &fb, std::vector<DeepSlice> &fill)
{
    const Slice& sampleCountSlice = fb.getSampleCountSlice ();

    fill.clear ();

    if (!sampleCountSlice.base)
    {
        throw IEX_NAMESPACE::ArgExc (
            "Invalid base pointer, please set a proper sample count slice.");
    }

    for (DeepFrameBuffer::ConstIterator j = fb.begin (); j != fb.end (); ++j)
    {
        const exr_attr_chlist_entry_t* curc = _ctxt->findChannel (
            partNumber, j.name ());

        if (!curc)
        {
            fill.push_back (j.slice ());
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
                    << _ctxt->fileName ()
                    << "\" are "
                       "not compatible with the frame buffer's "
                       "subsampling factors.");
    }
}


////////////////////////////////////////

#if ILMTHREAD_THREADING_ENABLED
void DeepScanLineInputFile::Data::LineBufferTask::execute ()
{
    try
    {
        _line->run_decode (
            *(_ifd->_ctxt),
            _ifd->partNumber,
            _outfb,
            _fby,
            _last_fby,
            _ifd->fill_list);
    }
    catch (std::exception &e)
    {
        _line_group->record_failure (e.what ());
    }
    catch (...)
    {
        _line_group->record_failure ("Unknown exception");
    }
}
#endif

////////////////////////////////////////

static exr_result_t
mem_skip_read_chunk (exr_decode_pipeline_t* decode)
{
    return EXR_ERR_SUCCESS;
}

void ScanLineProcess::run_mem_decode (
        exr_const_context_t ctxt,
        int pn,
        const char *rawdata,
        const DeepFrameBuffer *outfb,
        int fbY,
        int fbLastY,
        const std::vector<DeepSlice> &filllist)
{
    if (!first)
        throw IEX_NAMESPACE::ArgExc ("Expect single-use process");

    if (EXR_ERR_SUCCESS != exr_decoding_initialize (ctxt, pn, &cinfo, &decoder))
        throw IEX_NAMESPACE::IoExc ("Unable to initialize decode pipeline");

    decoder.decode_flags |= EXR_DECODE_NON_IMAGE_DATA_AS_POINTERS;
    decoder.decode_flags |= EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL;
    if (counts_only)
        decoder.decode_flags |= EXR_DECODE_SAMPLE_DATA_ONLY;

    update_pointers (outfb, fbY, fbLastY);

    if (EXR_ERR_SUCCESS !=
        exr_decoding_choose_default_routines (ctxt, pn, &decoder))
    {
        throw IEX_NAMESPACE::IoExc ("Unable to choose decoder routines");
    }
    decoder.read_fn = &mem_skip_read_chunk;
    // leave alloc sizes at 0 such that the decode pipeline doesn't
    // attempt to free, making it safe to cast const away
    decoder.packed_sample_count_table = const_cast<char*> (rawdata);
    rawdata += cinfo.sample_count_table_size;
    decoder.packed_buffer = const_cast<char*> (rawdata);

    last_decode_err = exr_decoding_run (ctxt, pn, &decoder);
    if (EXR_ERR_SUCCESS != last_decode_err)
        throw IEX_NAMESPACE::IoExc ("Unable to run decoder");

    copy_sample_count (outfb, fbY);

    if (counts_only)
        return;

    run_fill (outfb, fbY, filllist);
}

////////////////////////////////////////

void ScanLineProcess::run_decode (
    exr_const_context_t ctxt,
    int pn,
    const DeepFrameBuffer *outfb,
    int fbY,
    int fbLastY,
    const std::vector<DeepSlice> &filllist)
{
    last_decode_err = EXR_ERR_UNKNOWN;
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

    if (counts_only)
        decoder.decode_flags |= EXR_DECODE_SAMPLE_DATA_ONLY;
    else
        decoder.decode_flags = decoder.decode_flags & ~EXR_DECODE_SAMPLE_DATA_ONLY;

    update_pointers (outfb, fbY, fbLastY);

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
        throw IEX_NAMESPACE::IoExc ("Unable to run decoder");

    copy_sample_count (outfb, fbY);

    if (counts_only)
        return;

    run_fill (outfb, fbY, filllist);
}

////////////////////////////////////////

void ScanLineProcess::run_unpack (
    exr_const_context_t ctxt,
    int pn,
    const DeepFrameBuffer *outfb,
    int fbY,
    int fbLastY,
    const std::vector<DeepSlice> &filllist)
{
    update_pointers (outfb, fbY, fbLastY);

    copy_sample_count (outfb, fbY);

    if (counts_only)
        return;

    /* won't work for deep where we need to re-allocate the number of
     * samples but for scenario where we have separated sample count read
     * and deep sample alloc, should be fine to bypass pipe
     * and run the unpacker */
    if (decoder.chunk.unpacked_size > 0 && decoder.unpack_and_convert_fn)
    {
        last_decode_err = decoder.unpack_and_convert_fn (&decoder);
        if (EXR_ERR_SUCCESS != last_decode_err)
            throw IEX_NAMESPACE::IoExc ("Unable to run decoder");
    }

    run_fill (outfb, fbY, filllist);
}

////////////////////////////////////////

void ScanLineProcess::update_pointers (
    const DeepFrameBuffer *outfb, int fbY, int fbLastY)
{
    decoder.user_line_begin_skip = fbY - cinfo.start_y;
    decoder.user_line_end_ignore = 0;
    int64_t endY = (int64_t)cinfo.start_y + (int64_t)cinfo.height - 1;
    if ((int64_t)fbLastY < endY)
        decoder.user_line_end_ignore = (int32_t)(endY - fbLastY);

    if (counts_only)
        return;

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

        curchan.user_bytes_per_element = fbslice->sampleStride;
        curchan.user_data_type         = (exr_pixel_type_t)fbslice->type;
        curchan.user_pixel_stride      = fbslice->xStride;
        curchan.user_line_stride       = fbslice->yStride;

        ptr  = reinterpret_cast<uint8_t*> (fbslice->base);
        ptr += int64_t (cinfo.start_x) * int64_t (fbslice->xStride);
        ptr += int64_t (fbY) * int64_t (fbslice->yStride);

        curchan.decode_to_ptr = ptr;
    }
}

////////////////////////////////////////

void ScanLineProcess::run_fill (
    const DeepFrameBuffer *outfb,
    int fbY,
    const std::vector<DeepSlice> &filllist)
{
    for (auto& fills: filllist)
    {
        uint8_t*       ptr;

        if (fills.xSampling != 1 || fills.ySampling != 1)
            throw IEX_NAMESPACE::InputExc ("Expect sampling of 1");

        ptr  = reinterpret_cast<uint8_t*> (fills.base);
        ptr += int64_t (cinfo.start_x) * int64_t (fills.xStride);
        ptr += int64_t (fbY) * int64_t (fills.yStride);

        // TODO: update ImfMisc, lift fill type / value
        int stop = cinfo.start_y + cinfo.height - decoder.user_line_end_ignore;
        for ( int y = fbY; y < stop; ++y )
        {
            const int32_t* counts = decoder.sample_count_table;
            counts += ((int64_t) y - (int64_t) cinfo.start_y) * (int64_t) cinfo.width;

            uint8_t* outptr = ptr;
            for ( int sx = 0, ex = cinfo.width; sx < ex; ++sx )
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

void ScanLineProcess::copy_sample_count (
    const DeepFrameBuffer *outfb,
    int fbY)
{
    const Slice& scslice = outfb->getSampleCountSlice ();

    int     end = cinfo.height - decoder.user_line_end_ignore;
    int64_t xS = int64_t (scslice.xStride);
    int64_t yS = int64_t (scslice.yStride);

    for ( int y = decoder.user_line_begin_skip; y < end; ++y )
    {
        const int32_t* counts = decoder.sample_count_table + y * cinfo.width;
        uint8_t*       ptr;

        ptr = reinterpret_cast<uint8_t*> (scslice.base);
        ptr += int64_t (cinfo.start_x) * xS;
        ptr += (int64_t (fbY) + int64_t (y)) * yS;

        if (xS == sizeof(int32_t))
        {
            memcpy (ptr, counts, cinfo.width * sizeof(int32_t));
        }
        else
        {
            for ( int x = 0; x < cinfo.width; ++x )
            {
                *((int32_t *)ptr) = counts[x];
                ptr += xS;
            }
        }
    }
}

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
