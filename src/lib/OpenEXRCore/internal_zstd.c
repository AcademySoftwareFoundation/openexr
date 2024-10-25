/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr_compression.h>
#include "internal_compress.h"
#include "internal_decompress.h"
#include "internal_coding.h"
#include "internal_structs.h"
#include <assert.h>
#include "blosc2.h"

#define ERROR_MSG_V(pipeline, msg, ...)                                        \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, rv, msg, __VA_ARGS__);           \
    }
#define ERROR_MSG(pipeline, msg)                                               \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, rv, msg);                        \
    }

#define XTRA_MSGS

#ifdef XTRA_MSGS
#    define DBGV(msg, ...) fprintf (stderr, msg, __VA_ARGS__);
#    define DBG(msg, ...) fprintf (stderr, msg);
#else
#    define DBGV(msg, ...)
#    define DBG(msg, ...)
#endif

static const uint64_t EXR_HALF_PRECISION_SIZE   = 2;
static const uint64_t EXR_SINGLE_PRECISION_SIZE = 4;

uint64_t
exr_get_zstd_lines_per_chunk ()
{
    return 1;
}

/**
 * \brief Get the zstd level from the context
 *
 * \param encode in: the encode pipeline
 * \return the compression level
 */
static int
_get_zstd_level (exr_encode_pipeline_t* encode)
{
    // Get the compression level from the context
    int          zstd_level = 5; // default compression level
    exr_result_t rv         = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &zstd_level);
    if (rv != EXR_ERR_SUCCESS)
    {
        ERROR_MSG (
            encode, "Failed to get zstd compression level ! Defaulting to 5.");
    }
    return zstd_level;
}

/**
 * \brief compress a buffer using blosc zstd
 *
 * \param inPtr in: input buffer
 * \param inSize in: input buffer size
 * \param typeSize in: input buffer type size
 * \param outPtr out: output buffer
 * \param outPtrSize out: output buffer size
 * \param zstdLevel in: compression level
 * \return size of compressed buffer (in bytes)
 */
static uint64_t
_compress_ztsd_blosc_chunk (
    char* inPtr,
    int   inSize,
    int   typeSize,
    void* outPtr,
    int   outPtrSize,
    int   zstdLevel)
{
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize       = typeSize;
    // clevel 9 is about a 20% increase in compression compared to 5.
    // Decompression speed is unchanged.
    cparams.clevel    = zstdLevel;
    cparams.nthreads  = 1;
    cparams.compcode  = BLOSC_ZSTD;        // Codec
    cparams.splitmode = BLOSC_NEVER_SPLIT; // Split => multithreading,
                                           // not split better compression
    blosc2_storage storage = BLOSC2_STORAGE_DEFAULTS;
    storage.contiguous     = true;
    storage.cparams        = &cparams;

    blosc2_schunk* _schunk = blosc2_schunk_new (&storage);

    blosc2_schunk_append_buffer (_schunk, inPtr, inSize);

    uint8_t* buffer;
    bool     shouldFree = true;
    int64_t  size = blosc2_schunk_to_buffer (_schunk, &buffer, &shouldFree);

    if (size <= inSize && size <= outPtrSize && size > 0)
    {
        memcpy (outPtr, buffer, size);
    }
    else
    {
        memcpy (outPtr, inPtr, inSize);
        size = inSize; // We increased compression size
    }

    if (shouldFree) { free (buffer); }

    blosc2_schunk_free (_schunk);
    return size;
}

/**
 * \brief decompress a zstd blosc chunk
 *
 * if outPtrSize == 0, the function will malloc the output buffer.
 *
 * \param inPtr in: input buffer
 * \param inSize in: input buffer size
 * \param outPtr out: output buffer
 * \param outPtrSize out: output buffer size
 * \return size of decompressed buffer (in bytes)
 */
static uint64_t
_uncompress_ztsd_blosc_chunk (
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    blosc2_schunk* _schunk =
        blosc2_schunk_from_buffer ((uint8_t*) inPtr, inSize, true);
    if (_schunk == NULL)
    {
        fprintf (
            stderr,
            "\nERROR: blosc2_schunk_from_buffer failed (inSize = %llu)\n",
            inSize);
        return -1;
    }

    if (outPtrSize == 0) // we don't have any storage allocated
    {
        *outPtr    = malloc (_schunk->nbytes);
        outPtrSize = _schunk->nbytes;
    }

    int size = blosc2_schunk_decompress_chunk (_schunk, 0, *outPtr, outPtrSize);
    blosc2_schunk_free (_schunk);

    return size;
}

/**
 * \brief Returns the cumulative number of samples per line.
 *
 * \param width in: number of pixels per line
 * \param height in:: number of lines
 * \param sampleTableIsCumulative in: true if sampleCountTable is cumulative
 * \param sampleCountTable in: samples per pixel for chunk
 * \param cumSampsPerLine out: cumulative samples per line (pre-alllocated)
 */
void
_cumulative_samples_per_line (
    int        width,
    int        height,
    bool       sampleTableIsCumulative,
    const int* sampleCountTable,
    int*       cumSampsPerLine)
{
    cumSampsPerLine[0] = 0;

    if (!sampleCountTable)
    {
        // assume 1 sample per pixel if we get an empty table
        for (int y = 0; y < height; ++y)
            cumSampsPerLine[y + 1] = cumSampsPerLine[y] + width;
        return;
    }

    for (int y = 0; y < height; ++y)
    {
        cumSampsPerLine[y + 1] = cumSampsPerLine[y];
        for (int x = 0; x < width; ++x)
            cumSampsPerLine[y + 1] += sampleCountTable[y * width + x];
    }
}

/**
 * \brief returns start offsets for all planar channels,
 *
 * \param channelCount in: number of channels
 * \param channelsTypeSize in: type byte size per channel
 * \param bufSampleCount in: total number of samples in buffer
 * \param chOffsets out: buffer offsets array
 * \param halfSize out: size of half data
 * \param singleSize out: size of single data
 */
uint64_t
_channel_offsets (
    int        channelCount,
    const int* channelsTypeSize,
    int        bufSampleCount,
    uint64_t*  chOffsets,
    uint64_t*  halfSize,
    uint64_t*  singleSize)
{
    // count the number of half and single precision channels
    int n_half   = 0;
    int n_single = 0;
    for (int ch = 0; ch < channelCount; ++ch)
    {
        int size = channelsTypeSize[ch];
        if (size == EXR_HALF_PRECISION_SIZE)
            ++n_half;
        else
            ++n_single;
    }

    // map offsets to channel numbers
    int      nh           = 0;
    int      ns           = 0;
    uint64_t half_ch_size = bufSampleCount * EXR_HALF_PRECISION_SIZE;
    uint64_t out_split    = n_half * half_ch_size;
    for (int i = 0; i < channelCount; ++i)
    {
        if (channelsTypeSize[i] == EXR_HALF_PRECISION_SIZE)
        {
            chOffsets[i] = half_ch_size * nh;
            ++nh;
        }
        else if (channelsTypeSize[i] == EXR_SINGLE_PRECISION_SIZE)
        {
            chOffsets[i] = out_split + half_ch_size * 2 * ns;
            ++ns;
        }
    }
    if (halfSize) *halfSize = n_half * half_ch_size;
    if (singleSize) *singleSize = n_single * half_ch_size * 2;
    return out_split;
}

/**
 * \brief Unpack a scanline/tile buffer into a size-sorted single buffer.
 *
 * Half channels come first, followed by float/uint channels). outSplitPos marks
 * the begining of float/uint data.
 * The buffers contain per-channel planar (multi-line) data.
 * Supports deep files by handling arbitrary number of samples per pixel.
 *
 * Example: 2 lines of 3 pixels with half r, float g, half b, uint i channels:
 *
 * before:
 * [rh rh rh gs gs gs bh bh bh is is is rh rh rh gs gs gs bh bh bh is is is]
 * after:
 * [rh rh rh rh rh rh bh bh bh bh bh bh gs gs gs gs gs gs is is is is is is]
 * ^                                   ^
 * outPos                              outSplitPos
 *
 * \param encode in: the encoding pipeline struct
 * \param outHalfPtr out: pointer to half data
 * \param outHalfSize out: size of half data
 * \param outSinglePtr out: pointer to single data
 * \param outSingleSize out: size of single data
 */
void
_unpack_channels2 (
    exr_encode_pipeline_t* encode,       //
    void**                 outHalfPtr,   //
    uint64_t*              outHalfSize,  //
    void**                 outSinglePtr, //
    uint64_t*              outSingleSize //
)
{
    int lineCount = encode->chunk.height;

    // build the per-line cumulative sample table for this chunk.
    bool sampleCountTableIsCumulative =
        (encode->encode_flags & EXR_ENCODE_DATA_SAMPLE_COUNTS_ARE_INDIVIDUAL);
    int cumSampsPerLine[lineCount + 1];
    _cumulative_samples_per_line (
        encode->chunk.width,
        encode->chunk.height,
        sampleCountTableIsCumulative,
        encode->sample_count_table,
        cumSampsPerLine);

    int bufSampleCount = cumSampsPerLine[lineCount];

    // get the channels byte size
    int channelsTypeSize[encode->channel_count];
    for (int i = 0; i < encode->channel_count; ++i)
    {
        channelsTypeSize[i] = (int) encode->channels[i].bytes_per_element;
    }

    uint64_t chOffsets[encode->channel_count];
    uint64_t splitOffset = _channel_offsets (
        encode->channel_count,
        channelsTypeSize,
        bufSampleCount,
        chOffsets,
        outHalfSize,
        outSingleSize);

    char* inPos  = (char*) encode->packed_buffer;
    char* outPtr = (char*) encode->scratch_buffer_1;
    for (int ln = 0; ln < lineCount; ++ln)
    {
        for (int ch = 0; ch < encode->channel_count; ++ch)
        {
            int lineSampleCount =
                cumSampsPerLine[ln + 1] - (ln > 0 ? cumSampsPerLine[ln] : 0);
            uint64_t copySize = channelsTypeSize[ch] * lineSampleCount;
            char*    outPos   = outPtr + chOffsets[ch] +
                           cumSampsPerLine[ln] * channelsTypeSize[ch];
            memcpy (outPos, inPos, copySize);
            inPos += copySize;
        }
    }
    *outHalfPtr   = encode->scratch_buffer_1;
    *outSinglePtr = encode->scratch_buffer_1 + splitOffset;
}

/**
 * \brief compress a full buffer.
 *
 * \param encode in: the encoding pipeline
 * \param inSize in: size of packed input buffer
 * \param inHalfPtr in: pointer to half data
 * \param inHalfSize in: size of half data
 * \param inSinglePtr in: pointer to single data
 * \param inSingleSize in: size of single data
 * \param outPtr out: pointer to output buffer
 * \param outSize out: size of compressed output buffer
 * \return size of compressed output buffer
 */
uint64_t
_compress_zstd (
    exr_encode_pipeline_t* encode,
    uint64_t               inSize,
    void*                  inHalfPtr,
    uint64_t               inHalfSize,
    void*                  inSinglePtr,
    uint64_t               inSingleSize,
    void*                  outPtr,
    uint64_t*              outSize)
{
    exr_result_t rv;

    DBG ("_compress_zstd: ");

    // Get the compression level from the context
    int zstd_level = _get_zstd_level (encode);

    // compress buffers here
    uint64_t outPtrSize  = 0;
    char*    outPtrPos   = outPtr;
    void*    scratch     = malloc (inSize);
    char*    bufs[2]     = {inHalfPtr, inSinglePtr};
    uint64_t bufsSize[2] = {inHalfSize, inSingleSize};
    int bufsDataSize[2]  = {EXR_HALF_PRECISION_SIZE, EXR_SINGLE_PRECISION_SIZE};
    for (unsigned b = 0; b < 2; ++b)
    {
        // compress buffer section
        uint64_t compressedSize = 0;
        if (bufsSize[b] > 0)
        {
            compressedSize = _compress_ztsd_blosc_chunk (
                bufs[b],
                bufsSize[b],
                bufsDataSize[b],
                scratch,
                inSize,
                zstd_level);
            if (compressedSize <= 0)
            {
                rv = EXR_ERR_UNKNOWN;
                ERROR_MSG_V (
                    encode,
                    "Failed to compress zstd buffer ! byte size = %d",
                    bufsDataSize[b]);
            }
        }

        // always write buffer size
        bool     isSmaller = compressedSize < bufsSize[b];
        uint64_t outSize   = isSmaller ? (uint64_t) compressedSize
                                       : (uint64_t) bufsSize[b];
        memcpy (outPtrPos, &outSize, sizeof (uint64_t));
        outPtrPos += sizeof (uint64_t);
        outPtrSize += sizeof (uint64_t); // need to add sizes to the output
                                         // stream length
        if (b == 0) { DBGV (" half size = %llu ", compressedSize); }
        else { DBGV (" single size = %llu ", compressedSize); }

        // write buffer data if not empty
        if (outSize > 0)
        {
            memcpy (outPtrPos, isSmaller ? scratch : bufs[b], outSize);
            // update tracking vars
            outPtrSize += outSize;
            outPtrPos += outSize;
        }
    }
    free (scratch);

    *outSize = outPtrSize;

    DBGV (" total size = %llu\n", outPtrSize);

    return outPtrSize;
}

/**
 * \brief pack an unpacked buffer into a scanline/tile buffer.
 *
 * Half channels come first, followed by float/uint channels). outSplitPos marks
 * the begining of float/uint data.
 * The input buffers contain per-channel planar (multi-line) data.
 * Supports deep files by handling arbitrary number of samples per pixel.
 *
 * Example
 * 2 lines of 3 pixels with half r, float g, half b, uint i channels, 1 sample
 * per pixel (non-deep file).
 *
 * before:
 * [rh rh rh rh rh rh bh bh bh bh bh bh gs gs gs gs gs gs is is is is is is]
 *
 * after:
 * [rh rh rh gs gs gs bh bh bh is is is rh rh rh gs gs gs bh bh bh is is is]
 *
 * \param decode in: the decoding pipeline
 * \param inPtr in: pointer to input buffer
 * \param outPtr out: pointer to output buffer
 */
void
_pack_channels (
    exr_decode_pipeline_t* decode, //
    const void*            inPtr,  //
    void*                  outPtr)
{
    int lineCount = decode->chunk.height;

    int  sampleCountPerLineCum[lineCount + 1];
    bool sampleCountTableIsCumulative =
        (decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL);
    _cumulative_samples_per_line (
        decode->chunk.width,
        decode->chunk.height,
        sampleCountTableIsCumulative,
        decode->sample_count_table,
        sampleCountPerLineCum);

    int bufSampleCount = sampleCountPerLineCum[lineCount];

    int channelsTypeSize[decode->channel_count];
    for (int i = 0; i < decode->channel_count; ++i)
        channelsTypeSize[i] = (int) decode->channels[i].bytes_per_element;

    uint64_t chOffsets[decode->channel_count];
    uint64_t splitOffset = _channel_offsets (
        decode->channel_count,
        channelsTypeSize,
        bufSampleCount,
        chOffsets,
        NULL,
        NULL);

    char*    outPos        = (char*) outPtr;
    uint64_t totalByteSize = 0;
    for (int ln = 0; ln < lineCount; ++ln)
    {
        for (int ch = 0; ch < decode->channel_count; ++ch)
        {
            int lineSampleCount = sampleCountPerLineCum[ln + 1] -
                                  (ln > 0 ? sampleCountPerLineCum[ln] : 0);
            uint64_t copySize = channelsTypeSize[ch] * lineSampleCount;
            char*    inPos    = (char*) inPtr + chOffsets[ch] +
                          sampleCountPerLineCum[ln] * channelsTypeSize[ch];
            memcpy (outPos, inPos, copySize);
            outPos += copySize;
            // debug
            totalByteSize += copySize;
            assert (totalByteSize <= decode->unpacked_alloc_size);
        }
    }
}

/**
 * \brief decompress a zstd buffer.
 *
 * \param inPtr in: pointer to input buffer
 * \param outPtr in: pointer to output buffer
 * \param outPtrByteSize out: output buffer size
 * \return size of decompressed data buffer (in bytes)
 */
uint64_t
_uncompress_zstd (
    const char*    inPtr, //
    void*          outPtr,
    const uint64_t outPtrByteSize)
{
    DBG ("_uncompress_zstd: ");

    int   outSize        = 0;
    char* inPtrPos       = (char*) inPtr;
    int   decompSize     = outPtrByteSize;
    char* decompPtr      = outPtr;
    char* decompWritePos = decompPtr;

    for (int b = 0; b < 2; ++b)
    {
        // read compressed buffer size
        uint64_t compressedBufSize = 0;
        memcpy (&compressedBufSize, inPtrPos, sizeof (compressedBufSize));
        inPtrPos += sizeof (compressedBufSize); // move read position

        if (b == 0) { DBGV (" half size = %llu ", compressedBufSize); }
        else { DBGV (" single size = %llu ", compressedBufSize); }

        if (compressedBufSize == 0) continue;

        // read buffer
        const uint64_t decompressedSize = _uncompress_ztsd_blosc_chunk (
            inPtrPos,
            compressedBufSize,
            (void**) &decompWritePos,
            decompSize // 0 == ask function to allocate required memory
        );
        if (decompressedSize < 0)
            fprintf (stderr, "ERROR: bloc2 failed to decompress !!");
        outSize += decompressedSize;        // update reported size
        inPtrPos += compressedBufSize;      // move read position
        decompWritePos += decompressedSize; // move write position
    }

    DBGV (" total size = %d\n", outSize);

    return outSize;
}

/* Public functions --------------------------------------------------------- */

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;

    assert (encode->packed_bytes > 0);
    assert (encode->packed_buffer != NULL);

    bool isSampleTable =
        encode->packed_buffer == encode->packed_sample_count_table;

    if (isSampleTable)
    {
        int level = _get_zstd_level (encode);

        const uint64_t compressedSize = _compress_ztsd_blosc_chunk (
            encode->packed_buffer,
            encode->packed_bytes,
            EXR_SINGLE_PRECISION_SIZE,
            (void**) &encode->compressed_buffer,
            encode->compressed_bytes,
            level);

        if (compressedSize < 0)
        {
            rv = EXR_ERR_UNKNOWN;
            ERROR_MSG (encode, "blosc2 failed to compress sampleTable !!");
            return rv;
        }

        encode->compressed_bytes = compressedSize;
    }
    else
    {

        // allocate scratch to store the unpacked input buffer
        rv = internal_encode_alloc_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(encode->scratch_buffer_1),
            &(encode->scratch_alloc_size_1),
            encode->packed_bytes);
        if (rv != EXR_ERR_SUCCESS)
        {
            ERROR_MSG_V (
                encode,
                "Failed to allocate scratch buffer 1 for zstd (%" PRIu64
                " bytes)",
                encode->packed_bytes);
            return rv;
        }

        void *   bufHalfPtr = NULL, *bufSinglePtr = NULL;
        uint64_t bufHalfSize = 0, bufSingleSize = 0;
        _unpack_channels2 (
            encode, &bufHalfPtr, &bufHalfSize, &bufSinglePtr, &bufSingleSize);
        assert (bufHalfPtr != NULL && bufSinglePtr != NULL);

        uint64_t compressedSize = _compress_zstd (
            encode,
            encode->scratch_alloc_size_1,
            bufHalfPtr,
            bufHalfSize,
            bufSinglePtr,
            bufSingleSize,
            encode->compressed_buffer,
            (uint64_t*) &(encode->compressed_bytes));

        // sanity check
        if (compressedSize < 0)
        {
            rv = EXR_ERR_UNKNOWN;
            ERROR_MSG (encode, "blosc2 failed to compress pixel samples !!");
            return rv;
        }
    }

    return EXR_ERR_SUCCESS;
}

exr_result_t
internal_exr_undo_zstd (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv;

    if (comp_buf_size == 0 || compressed_data == NULL)
    {
        decode->bytes_decompressed = 0;
        return EXR_ERR_SUCCESS;
    }

    bool isSampleTable = compressed_data == decode->sample_count_table;

    if (isSampleTable)
    {
        uint64_t uncompressedSize = _uncompress_zstd (
            compressed_data, uncompressed_data, uncompressed_size);
        if (uncompressed_size != uncompressedSize)
        {
            return EXR_ERR_CORRUPT_CHUNK;
        }
    }
    else
    {
        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(decode->scratch_buffer_1),
            &(decode->scratch_alloc_size_1),
            uncompressed_size);
        if (rv != EXR_ERR_SUCCESS)
        {
            ERROR_MSG_V (
                decode,
                "Failed to allocate scratch buffer 1 for zstd (%" PRIu64
                " bytes)",
                uncompressed_size);
            return rv;
        }

        uint64_t uncompressedSize = _uncompress_zstd (
            compressed_data, decode->scratch_buffer_1, uncompressed_size);

        _pack_channels (decode, decode->scratch_buffer_1, uncompressed_data);

        if (uncompressed_size != uncompressedSize)
        {
            return EXR_ERR_CORRUPT_CHUNK;
        }
    }

    return EXR_ERR_SUCCESS;
}