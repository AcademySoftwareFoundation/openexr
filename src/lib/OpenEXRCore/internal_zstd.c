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

#define ERROR_MSG(pipeline, msg)                                               \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, rv, msg);                        \
    }
#define ERROR_MSGV(pipeline, msg, ...)                                         \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, rv, msg, __VA_ARGS__);           \
    }
#define RETURN_ERROR(pipeline, err_code, msg)                                  \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, err_code, msg);                  \
        return err_code;                                                       \
    }
#define RETURN_ERRORV(pipeline, err_code, msg, ...)                            \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, err_code, msg, __VA_ARGS__);     \
        return err_code;                                                       \
    }

// #define XTRA_MSGS
#ifdef XTRA_MSGS
#    define DBGV(msg, ...) fprintf (stderr, msg, __VA_ARGS__);
#    define DBG(msg, ...) fprintf (stderr, msg);
#else
#    define DBGV(msg, ...)
#    define DBG(msg, ...)
#endif

#define EXR_HALF_PRECISION_SIZE 2
#define EXR_SINGLE_PRECISION_SIZE 4

// ----------------------------------------------------------------------------

// FIXME: most fields could be pointers to avoid copying

struct chan_infos
{
    int8_t  type_size;
    int32_t x_sampling;
    int32_t y_sampling;
    int32_t byte_size;
    int32_t offset;
};
typedef struct chan_infos chan_infos_t;

struct zstd_data
{
    void*         buffers[2];      // pointers to half and float buffers
    int32_t       buffers_size[2]; // byte size of half and float buffers
    bool          is_deep;         // are we handling deep data ?
    int32_t       width;           // buffer pixels width
    int32_t       height;          // buffer pixels height / number of lines
    int32_t       channel_count;   // number of channels
    chan_infos_t* channels;        // array of channels infos
    int32_t*      sample_count_per_line_cumulative;
};
typedef struct zstd_data zstd_data_t;

static exr_result_t
_new_encoding_zstd_data (exr_encode_pipeline_t* encode)
{
    encode->encoding_user_data = malloc (sizeof (zstd_data_t));
    if (!encode->encoding_user_data)
        RETURN_ERROR (
            encode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private encoding data ! (zstd_data_t)");

    zstd_data_t* zstd_data = (zstd_data_t*) encode->encoding_user_data;

    zstd_data->buffers[0] = NULL;
    zstd_data->buffers[1] = NULL;

    zstd_data->buffers_size[0] = 0;
    zstd_data->buffers_size[1] = 0;

    zstd_data->is_deep       = encode->chunk.sample_count_table_size > 0;
    zstd_data->width         = encode->chunk.width;
    zstd_data->height        = encode->chunk.height;
    zstd_data->channel_count = encode->channel_count;

    zstd_data->channels =
        malloc (sizeof (chan_infos_t) * encode->channel_count);
    if (!zstd_data->channels)
        RETURN_ERROR (
            encode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private encoding data ! (chan_infos_t)");
    for (int i = 0; i < encode->channel_count; ++i)
    {
        zstd_data->channels[i].type_size =
            encode->channels[i].bytes_per_element;
        zstd_data->channels[i].x_sampling = encode->channels[i].x_samples;
        zstd_data->channels[i].y_sampling = encode->channels[i].y_samples;
        zstd_data->channels[i].byte_size  = 0;
        zstd_data->channels[i].offset     = 0;
    }

    zstd_data->sample_count_per_line_cumulative =
        malloc (sizeof (int) * (encode->chunk.height + 1));
    if (!zstd_data->sample_count_per_line_cumulative)
        RETURN_ERROR (
            encode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private encoding data ! "
            "(sample_count_per_line_cumulative)");

    return EXR_ERR_SUCCESS;
}

static exr_result_t
_free_encoding_zstd_data (exr_encode_pipeline_t* encode)
{
    zstd_data_t* zstd_data = (zstd_data_t*) encode->encoding_user_data;
    free (zstd_data->channels);
    free (zstd_data->sample_count_per_line_cumulative);
    free (zstd_data);
    encode->encoding_user_data = NULL;
    return EXR_ERR_SUCCESS;
}

static exr_result_t
_new_decoding_zstd_data (exr_decode_pipeline_t* decode)
{
    decode->decoding_user_data = malloc (sizeof (zstd_data_t));
    if (!decode->decoding_user_data)
        RETURN_ERROR (
            decode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private decoding data ! (zstd_data_t)");

    zstd_data_t* zstd_data = (zstd_data_t*) decode->decoding_user_data;

    zstd_data->buffers[0] = NULL; // pointer doesn't belong to us
    zstd_data->buffers[1] = NULL; // pointer doesn't belong to us

    zstd_data->buffers_size[0] = 0;
    zstd_data->buffers_size[1] = 0;

    zstd_data->is_deep       = decode->chunk.sample_count_table_size > 0;
    zstd_data->width         = decode->chunk.width;
    zstd_data->height        = decode->chunk.height;
    zstd_data->channel_count = decode->channel_count;

    zstd_data->channels =
        malloc (sizeof (chan_infos_t) * decode->channel_count);
    if (!zstd_data->channels)
        RETURN_ERROR (
            decode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private decoding data ! (chan_infos_t)");
    for (int i = 0; i < decode->channel_count; ++i)
    {
        zstd_data->channels[i].type_size =
            decode->channels[i].bytes_per_element;
        zstd_data->channels[i].x_sampling = decode->channels[i].x_samples;
        zstd_data->channels[i].y_sampling = decode->channels[i].y_samples;
        zstd_data->channels[i].byte_size  = 0;
        zstd_data->channels[i].offset     = 0;
    }

    zstd_data->sample_count_per_line_cumulative =
        malloc (sizeof (int) * (decode->chunk.height + 1));
    if (!zstd_data->sample_count_per_line_cumulative)
        RETURN_ERROR (
            decode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate private decoding data ! "
            "(sample_count_per_line_cumulative)");
    return EXR_ERR_SUCCESS;
}

static exr_result_t
_free_decoding_zstd_data (exr_decode_pipeline_t* decode)
{
    zstd_data_t* zstd_data = (zstd_data_t*) decode->decoding_user_data;
    free (zstd_data->channels);
    free (zstd_data->sample_count_per_line_cumulative);
    free (zstd_data);
    decode->decoding_user_data = NULL;
    return EXR_ERR_SUCCESS;
}

// ----------------------------------------------------------------------------

static int
_divp (int x, int y)
{
    return (x >= 0) ? ((y >= 0) ? (x / y) : -(x / -y))
                    : ((y >= 0) ? -((y - 1 - x) / y) : ((-y - 1 - x) / -y));
}

static int
_modp (int x, int y)
{
    return x - y * _divp (x, y);
}

// ----------------------------------------------------------------------------

#ifndef NDEBUG
/**
 * \brief debug function to inspect the blosc header in the debugger.
 *        code copied from blosc.c
 *
 * \param data
 * \param size
 */
static void
_check_blosc_header (const void* data, size_t size)
{
    typedef struct blosc_header_s
    {
        uint8_t version;
        uint8_t versionlz;
        uint8_t flags;
        uint8_t typesize;
        int32_t nbytes;
        int32_t blocksize;
        int32_t cbytes;
        // Extended Blosc2 header
        uint8_t filter_codes[BLOSC2_MAX_FILTERS];
        uint8_t udcompcode;
        uint8_t compcode_meta;
        uint8_t filter_meta[BLOSC2_MAX_FILTERS];
        uint8_t reserved2;
        uint8_t blosc2_flags;
    } blosc_header;
    blosc_header header;
    memset (&header, 0, sizeof (blosc_header));
    memcpy (&header, data, BLOSC_MIN_HEADER_LENGTH);
    int v = header.version;
}
#else
static void
_check_blosc_header (const void* data, size_t size)
{}
#endif

// ----------------------------------------------------------------------------

/**
 * \brief Get the zstd level from the context
 *
 * \param encode in: the encode pipeline
 * \return the compression level
 */
static int32_t
_get_zstd_level (const exr_encode_pipeline_t* encode)
{
    // Get the compression level from the context
    int32_t      zstd_level = 5; // default compression level
    exr_result_t rv         = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &zstd_level);
    if (rv != EXR_ERR_SUCCESS)
    {
        ERROR_MSGV (
            encode,
            "[zstd]  Failed to get compression level ! Defaulting to %d.",
            zstd_level);
    }
    return zstd_level;
}

/**
 * \brief Read the buffer size from the buffer (4.29 GB max) and move
 * the read position to the start of buffer data.
 *
 * \param data  the buffer
 * \return      int32_t the buffer size in bytes
 */
static int32_t
_read_buffer_size (char** src)
{
    int32_t bufByteSize = 0;
    memcpy (&bufByteSize, *src, sizeof (bufByteSize));
    assert (bufByteSize <= INT32_MAX && "read buffer size too large");
    *src += sizeof (bufByteSize); // move read position
    return bufByteSize;
}

static void
_read_buffer_data (char** src, int32_t bufByteSize, void* dst)
{
    if (bufByteSize > 0)
    {
        memcpy (dst, *src, bufByteSize);
        *src += bufByteSize;
    }
}

/**
 * \brief Write the buffer size to the buffer (4.29 GB max) and move the write
 * position to the start of buffer data.
 *
 * \param data          the buffer
 * \param bufByteSize   the buffer size in bytes
 */
static void
_write_buffer_size (char** dst, const int32_t bufByteSize)
{
    assert (bufByteSize <= INT32_MAX && "write buffer size too large");
    memcpy (*dst, &bufByteSize, sizeof (bufByteSize));
    *dst += sizeof (bufByteSize); // move write position
}

static void
_write_buffer_data (char** dst, const int32_t bufByteSize, const void* src)
{
    assert (bufByteSize <= INT32_MAX && "write data buffer size too large");
    if (bufByteSize > 0)
    {
        memcpy (*dst, src, bufByteSize);
        *dst += bufByteSize; // move write position
    }
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
static int32_t
_compress_ztsd_blosc_chunk (
    char*   inPtr,
    int32_t inSize,
    int8_t  typeSize,
    void*   outPtr,
    int32_t outPtrSize,
    int     zstdLevel)
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

    blosc2_context* cctx = blosc2_create_cctx (cparams);
    int32_t         size =
        blosc2_compress_ctx (cctx, inPtr, inSize, outPtr, outPtrSize);
    blosc2_free_ctx (cctx);
    DBGV (
        " blosc2_compress_ctx size: %.02f%%  (%d / %d) ",
        (float) size / (float) inSize * 100.f,
        size,
        inSize);
    _check_blosc_header (outPtr, outPtrSize);
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
static int32_t
_uncompress_ztsd_blosc_chunk (
    const char* inPtr, int32_t inSize, void** outPtr, int32_t outPtrSize)
{
    blosc2_dparams dparams = BLOSC2_DPARAMS_DEFAULTS;
    dparams.nthreads       = 1;
    blosc2_context* dctx   = blosc2_create_dctx (dparams);
    int32_t         size =
        blosc2_decompress_ctx (dctx, inPtr, inSize, *outPtr, outPtrSize);
    DBGV (
        " blosc2_decompress_ctx size: %.02f%%  (%d / %d) ",
        (float) size / (float) inSize * 100.f,
        size,
        inSize);

    blosc2_free_ctx (dctx);
    _check_blosc_header (inPtr, inSize);
    return size;
}

/**
 * \brief Returns the cumulative number of samples per line, irrespective of
 *        subsampling.
 *
 * \param width in: number of pixels per line
 * \param height in:: number of lines
 * \param sampleTableIsCumulative in: true if sampleCountTable is cumulative
 * \param sampleCountTable in: samples per pixel for chunk
 * \param cumSampsPerLine out: cumulative samples per line (pre-alllocated)
 */
static void
_cumulative_samples_per_line (
    zstd_data_t*   zstd_data,
    bool           sampleTableIsCumulative,
    const int32_t* sampleCountTable)
{
    int32_t* table = zstd_data->sample_count_per_line_cumulative;
    table[0]       = 0;

    if (!sampleCountTable)
    {
        // assume 1 sample per pixel if we get an empty table (non-deep data)
        for (int y = 0; y < zstd_data->height; ++y)
            table[y + 1] = table[y] + zstd_data->width;
        return;
    }

    // deep data: arbitrary number of samples per pixel
    if (sampleTableIsCumulative)
    {
        for (int y = 0; y < zstd_data->height; ++y)
        {
            table[y + 1] = sampleCountTable[y + 1];
        }
    }
    else
    {
        for (int y = 0; y < zstd_data->height; ++y)
        {
            table[y + 1] = table[y];
            for (int x = 0; x < zstd_data->width; ++x)
                table[y + 1] += sampleCountTable[y * zstd_data->width + x];
        }
    }
}

static int32_t
_active_pixels_for_line (
    int32_t line_number, int32_t width, int32_t x_sampling, int32_t y_sampling)
{
    // Check if this line is active based on y_sampling
    if ((line_number % y_sampling) != 0) return 0;

    // For active lines, return subsampled width
    return (width + x_sampling - 1) / x_sampling;
}

static int32_t
_num_subsampled_pixels (
    int32_t width, int32_t height, int32_t x_sampling, int32_t y_sampling)
{
    // Round up division to handle non-perfect multiples
    int32_t effective_width  = (width + x_sampling - 1) / x_sampling;
    int32_t effective_height = (height + y_sampling - 1) / y_sampling;
    return effective_width * effective_height;
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
static void
_channel_offsets (zstd_data_t* zstd_data)
{
    // count the number of half precision channels and precompute the number of
    // pixels per channel for the whole buffer.
    int32_t n_half = 0;
    int32_t sample_counts[zstd_data->channel_count];
    for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
    {
        if (zstd_data->channels[ch].type_size == EXR_HALF_PRECISION_SIZE)
            ++n_half;
        if (zstd_data->is_deep)
        {
            // deep data: subsampling is not supported, i.e. same value for
            // all channels.
            sample_counts[ch] =
                zstd_data->sample_count_per_line_cumulative[zstd_data->height];
        }
        else
            sample_counts[ch] = _num_subsampled_pixels (
                zstd_data->width,
                zstd_data->height,
                zstd_data->channels[ch].x_sampling,
                zstd_data->channels[ch].y_sampling);
    }

    // sort channels so half precision comes before single precision
    int32_t ch_indices[zstd_data->channel_count];
    int32_t ih = 0, is = n_half;
    for (int32_t i = 0; i < zstd_data->channel_count; ++i)
    {
        if (zstd_data->channels[i].type_size == EXR_HALF_PRECISION_SIZE)
            ch_indices[ih++] = i;
        else
            ch_indices[is++] = i;
    }

    // store offsets for each channel
    int32_t h_size = 0;
    for (int32_t i = 0; i < n_half; i++)
    {
        int32_t idx                     = ch_indices[i];
        zstd_data->channels[idx].offset = h_size;
        zstd_data->channels[idx].byte_size =
            sample_counts[idx] * EXR_HALF_PRECISION_SIZE;
        h_size += zstd_data->channels[idx].byte_size;
    }

    int32_t s_size = h_size;
    for (int32_t i = n_half; i < zstd_data->channel_count; i++)
    {
        int32_t idx                     = ch_indices[i];
        zstd_data->channels[idx].offset = s_size;
        zstd_data->channels[idx].byte_size =
            sample_counts[idx] * EXR_SINGLE_PRECISION_SIZE;
        s_size += zstd_data->channels[idx].byte_size;
    }

    zstd_data->buffers_size[0] = h_size;
    zstd_data->buffers_size[1] = s_size - h_size;
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
 */
// static void
// _unpack_channels (
//     zstd_data_t*   zstd_data,
//     const void*    inPtr,
//     const uint64_t inSize,
//     void*          outPtr)
// {
//     _channel_offsets (zstd_data);

//     char*   inPos      = (char*) inPtr;
//     char*   outPtrPos  = (char*) outPtr;
//     int32_t outPtrSize = 0;
//     int32_t chWritePos[zstd_data->channel_count];
//     memset (chWritePos, 0, sizeof (chWritePos));
//     for (int32_t ln = 0; ln < zstd_data->height; ++ln)
//     {
//         for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
//         {
//             int32_t lineSampleCount =
//                 zstd_data->is_deep
//                     ? zstd_data->sample_count_per_line_cumulative[ln + 1] -
//                           (ln > 0
//                                ? zstd_data->sample_count_per_line_cumulative[ln]
//                                : 0)
//                     : _active_pixels_for_line (
//                           ln,
//                           zstd_data->width,
//                           zstd_data->channels[ch].x_sampling,
//                           zstd_data->channels[ch].y_sampling);
//             size_t copySize =
//                 zstd_data->channels[ch].type_size * lineSampleCount;
//             char* outPos =
//                 zstd_data->is_deep
//                     ? outPtrPos + zstd_data->channels[ch].offset +
//                           zstd_data->sample_count_per_line_cumulative[ln] *
//                               zstd_data->channels[ch].type_size
//                     : outPtrPos + zstd_data->channels[ch].offset +
//                           chWritePos[ch];
//             outPtrSize += copySize;
//             if (zstd_data->channels[ch].x_sampling > 1)
//                 DBGV (
//                     "unpack:  lw=%d  ch=%d  lsc=%d  cs=%zu  total=%d\n",
//                     zstd_data->width,
//                     ch,
//                     lineSampleCount,
//                     copySize,
//                     outPtrSize);
//             assert (outPtrSize <= inSize && "out of bounds");
//             memcpy (outPos, inPos, copySize);
//             inPos += copySize;
//             chWritePos[ch] += copySize;
//         }
//     }
//     zstd_data->buffers[0] = outPtr;
//     zstd_data->buffers[1] = outPtr + zstd_data->buffers_size[0];
// }
static void
_unpack_channels (
    zstd_data_t*   zstd_data,
    const void*    inPtr,
    const uint64_t inSize,
    void*          outPtr)
{
    _channel_offsets (zstd_data);

    // Pre-compute line sample counts per channel
    int32_t* lineSampleCounts = malloc (
        zstd_data->height * zstd_data->channel_count * sizeof (int32_t));
    for (int32_t ln = 0; ln < zstd_data->height; ++ln)
    {
        for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
        {
            int32_t idx = ln * zstd_data->channel_count + ch;
            lineSampleCounts[idx] =
                zstd_data->is_deep
                    ? zstd_data->sample_count_per_line_cumulative[ln + 1] -
                          (ln > 0
                               ? zstd_data->sample_count_per_line_cumulative[ln]
                               : 0)
                    : _active_pixels_for_line (
                          ln,
                          zstd_data->width,
                          zstd_data->channels[ch].x_sampling,
                          zstd_data->channels[ch].y_sampling);
        }
    }

    char*    inPos      = (char*) inPtr;
    char*    outPtrPos  = (char*) outPtr;
    int32_t  outPtrSize = 0;
    int32_t* chWritePos = calloc (zstd_data->channel_count, sizeof (int32_t));

    // Main unpacking loop
    for (int32_t ln = 0; ln < zstd_data->height; ++ln)
    {
        for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
        {
            const int32_t lineSampleCount =
                lineSampleCounts[ln * zstd_data->channel_count + ch];
            const size_t copySize =
                zstd_data->channels[ch].type_size * lineSampleCount;

            char* outPos =
                zstd_data->is_deep
                    ? outPtrPos + zstd_data->channels[ch].offset +
                          zstd_data->sample_count_per_line_cumulative[ln] *
                              zstd_data->channels[ch].type_size
                    : outPtrPos + zstd_data->channels[ch].offset +
                          chWritePos[ch];

            // DBGV (
            //     "unpack:  lw=%d  ch=%d  lsc=%d  cs=%zu  total=%d\n",
            //     zstd_data->width,
            //     ch,
            //     lineSampleCount,
            //     copySize,
            //     outPtrSize);
            assert (outPtrSize <= inSize && "out of bounds");

            memcpy (outPos, inPos, copySize);
            inPos += copySize;
            chWritePos[ch] += copySize;
            outPtrSize += copySize;
        }
    }

    free (lineSampleCounts);
    free (chWritePos);

    zstd_data->buffers[0] = outPtr;
    zstd_data->buffers[1] = outPtr + zstd_data->buffers_size[0];
}

/**
 * \brief compress a full buffer.
 *
 * \param encode in: the encoding pipeline
 * \param inSize in: size of packed input buffer
 * \param outPtr out: pointer to output buffer
 * \param outSize out: size of compressed output buffer
 * \return exr_result_t: EXR_ERR_SUCCESS or EXR_ERR_OUT_OF_MEMORY
 */
static exr_result_t
_compress_zstd (
    const exr_encode_pipeline_t* encode,
    const size_t                 inSize,
    void*                        outPtr,
    size_t*                      outPtrSize)
{
    exr_result_t       rv        = EXR_ERR_SUCCESS;
    const zstd_data_t* zstd_data = (zstd_data_t*) encode->encoding_user_data;

    // Get the compression level from the context
    const int32_t zstd_level = _get_zstd_level (encode);

    DBG ("_compress_zstd:  ");

    // compress buffers here
    *outPtrSize    = 0;
    char* writePos = (char*) outPtr;
    void* scratch  = malloc (inSize); // FIXME: use pipeline scratch buffer
    if (scratch == NULL)
        RETURN_ERROR (
            encode,
            EXR_ERR_OUT_OF_MEMORY,
            "[zstd]  Failed to allocate compression scratch buffer");
    int8_t bufsDataSize[2] = {
        EXR_HALF_PRECISION_SIZE, EXR_SINGLE_PRECISION_SIZE};

    for (uint32_t b = 0; b < 2; ++b)
    {
        // compress buffer section
        int32_t compressedSize = 0; // may be negative
        if (zstd_data->buffers_size[b] > 0)
        {
            compressedSize = _compress_ztsd_blosc_chunk (
                zstd_data->buffers[b],
                zstd_data->buffers_size[b],
                bufsDataSize[b],
                scratch,
                inSize,
                zstd_level);

            if (compressedSize < 0)
            {
                free (scratch);
                RETURN_ERRORV (
                    encode,
                    EXR_ERR_COMPRESSION_FAILED,
                    "[zstd]  Failed to compress buffer ! byte size = %d",
                    bufsDataSize[b]);
            }

            if (compressedSize >= zstd_data->buffers_size[b])
            {
                // zstd failed to shrink the data
                DBGV (
                    "[ UNCOMPRESSED BUFFER !  (%s) %d <= %d ] ",
                    b == 0 ? "half" : "single",
                    zstd_data->buffers_size[b],
                    compressedSize);
                // we store the uncompressed data
                compressedSize = zstd_data->buffers_size[b];
            }
        }

        // compute storable data size
        // use a signed int32_t to store the size of the compressed buffer (+/- 2.1 GB max)
        bool    isSmaller = compressedSize < zstd_data->buffers_size[b];
        int32_t outSize   = isSmaller ? compressedSize
                                      : zstd_data->buffers_size[b];

        // check if we have enough space to store the compressed buffer
        size_t projected_size = *outPtrSize + outSize + sizeof (uint32_t);
        if (projected_size >= inSize)
        {
            DBG (" Not compressing !\n");
            free (scratch);
            return EXR_ERR_COMPRESSION_FAILED;
        }

        // Write header (buffer size).
        // NOTE: If the data has NOT been compressed, we store a negative size
        //       and the decompression function will just memcpy the buffer.
        _write_buffer_size (&writePos, isSmaller ? outSize : -outSize);
        *outPtrSize += sizeof (outSize); // add size header
        *outPtrSize += (size_t) outSize; // add data size - can be 0
        _write_buffer_data (
            &writePos, outSize, isSmaller ? scratch : zstd_data->buffers[b]);

        if (b == 0) { DBGV (" half_size = %d ", compressedSize); }
        else { DBGV (" single_size = %d ", compressedSize); }
    }
    free (scratch);

    assert (*outPtrSize < inSize);

    // if (*outPtrSize >= inSize) rv = EXR_ERR_COMPRESSION_FAILED;

    DBGV (
        " total_size = %zu / %zu  %s\n",
        *outPtrSize,
        inSize,
        *outPtrSize >= inSize ? "TOO_BIG" : "");

    return rv;
}

static void
_decoding_vars (exr_decode_pipeline_t* decode)
{
    zstd_data_t* zstd_data = (zstd_data_t*) decode->decoding_user_data;

    int32_t lineCount = decode->chunk.height;

    bool sampleCountTableIsCumulative =
        (decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL);

    _cumulative_samples_per_line (
        zstd_data, sampleCountTableIsCumulative, decode->sample_count_table);

    _channel_offsets (zstd_data);
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
// static void
// _pack_channels (
//     zstd_data_t*   zstd_data,
//     const void*    inPtr,
//     void*          outPtr,
//     const uint64_t outPtrSize)
// {
//     char*   outPos        = (char*) outPtr;
//     size_t  totalByteSize = 0;
//     for (int32_t ln = 0; ln < zstd_data->height; ++ln)
//     {
//         for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
//         {
//             int32_t lineSampleCount =
//                 zstd_data->sample_count_per_line_cumulative[ln + 1] -
//                 (ln > 0 ? zstd_data->sample_count_per_line_cumulative[ln] : 0);
//             size_t copySize =
//                 zstd_data->channels[ch].type_size * lineSampleCount;
//             char* inPos = (char*) inPtr + zstd_data->channels[ch].offset +
//                           zstd_data->sample_count_per_line_cumulative[ln] *
//                               zstd_data->channels[ch].type_size;
//             memcpy (outPos, inPos, copySize);
//             outPos += copySize;

//             // validation
//             totalByteSize += copySize;
//             assert (totalByteSize <= outPtrSize);
//         }
//     }
//     assert (totalByteSize == outPtrSize);
// }
static void
_pack_channels (
    zstd_data_t*   zstd_data,
    const void*    inPtr,
    void*          outPtr,
    const uint64_t outPtrSize)
{
    // Pre-compute line sample counts per channel
    int32_t* lineSampleCounts = malloc (
        zstd_data->height * zstd_data->channel_count * sizeof (int32_t));
    for (int32_t ln = 0; ln < zstd_data->height; ++ln)
    {
        for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
        {
            int32_t idx = ln * zstd_data->channel_count + ch;
            lineSampleCounts[idx] =
                zstd_data->is_deep
                    ? zstd_data->sample_count_per_line_cumulative[ln + 1] -
                          (ln > 0
                               ? zstd_data->sample_count_per_line_cumulative[ln]
                               : 0)
                    : _active_pixels_for_line (
                          ln,
                          zstd_data->width,
                          zstd_data->channels[ch].x_sampling,
                          zstd_data->channels[ch].y_sampling);
        }
    }

    char*    outPos        = (char*) outPtr;
    size_t   totalByteSize = 0;
    int32_t* chReadPos = calloc (zstd_data->channel_count, sizeof (int32_t));

    // Main packing loop
    for (int32_t ln = 0; ln < zstd_data->height; ++ln)
    {
        for (int32_t ch = 0; ch < zstd_data->channel_count; ++ch)
        {
            const int32_t lineSampleCount =
                lineSampleCounts[ln * zstd_data->channel_count + ch];
            const size_t copySize =
                zstd_data->channels[ch].type_size * lineSampleCount;

            const char* inPos =
                zstd_data->is_deep
                    ? (const char*) inPtr + zstd_data->channels[ch].offset +
                          zstd_data->sample_count_per_line_cumulative[ln] *
                              zstd_data->channels[ch].type_size
                    : (const char*) inPtr + zstd_data->channels[ch].offset +
                          chReadPos[ch];

            memcpy (outPos, inPos, copySize);
            outPos += copySize;
            chReadPos[ch] += copySize;
            totalByteSize += copySize;
            assert (totalByteSize <= outPtrSize);
        }
    }

    free (lineSampleCounts);
    assert (totalByteSize == outPtrSize);
}

/**
 * \brief decompress a zstd buffer.
 *
 * \param inPtr in: pointer to input buffer
 * \param outPtr in: pointer to output buffer
 * \param outPtrByteSize out: output buffer size
 * \return size of decompressed data buffer (in bytes)
 */
static exr_result_t
_uncompress_zstd (
    exr_decode_pipeline_t* decode,
    const char*            inPtr,
    void*                  outPtr,
    const int32_t          outPtrByteSize,
    const int32_t          decompressedByteSizes[2])
{
    DBG ("_uncompress_zstd: ");

    int32_t outSize        = 0;
    char*   inPtrPos       = (char*) inPtr;
    int32_t decompSize     = outPtrByteSize;
    char*   decompPtr      = outPtr;
    char*   decompWritePos = decompPtr;

    for (int32_t b = 0; b < 2; ++b)
    {
        int32_t compressedBufSize = _read_buffer_size (&inPtrPos);

        if (b == 0) { DBGV (" half size = %d ", compressedBufSize); }
        else { DBGV (" single size = %d ", compressedBufSize); }

        // there is no data of the current type
        if (compressedBufSize == 0) continue;

        // read buffer
        int32_t expectedSize = decompressedByteSizes ? decompressedByteSizes[b]
                                                     : 0;
        int32_t decompressedSize = 0;
        if (compressedBufSize > 0)
        {
            decompressedSize = _uncompress_ztsd_blosc_chunk (
                inPtrPos,
                compressedBufSize,
                (void**) &decompWritePos,
                decompSize);

            if (decompressedSize < 0)
                RETURN_ERROR (
                    decode,
                    EXR_ERR_DECOMPRESSION_FAILED,
                    "[zstd]  bloc2 failed to decompress !")
        }
        else if (compressedBufSize < 0)
        {
            // we stored the original data because zstd didn't manage to compress it.
            compressedBufSize = -compressedBufSize; // make it positive
            decompressedSize  = compressedBufSize;
            memcpy (decompWritePos, inPtrPos, compressedBufSize);
        }

        if (decompressedSize != expectedSize)
            RETURN_ERRORV (
                decode,
                EXR_ERR_CORRUPT_CHUNK,
                "[zstd]  bloc2 decompressed size %d != expected %d !\n",
                decompressedSize,
                expectedSize);

        // book-keeping
        outSize += decompressedSize;        // update reported size
        inPtrPos += compressedBufSize;      // move read position
        decompWritePos += decompressedSize; // move write position
    }

    DBGV (" total size = %d\n", outSize);

    // return outSize;
    return EXR_ERR_SUCCESS;
}

/* Public functions --------------------------------------------------------- */

/**
 * \brief Compression function called by the C API.
 *
 * \param encode pointer to the encoding pipeline.
 * \return exr_result_t
 */
exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    assert (encode->packed_bytes > 0);
    assert (encode->packed_buffer != NULL);

    bool isSampleTable =
        encode->packed_buffer == encode->packed_sample_count_table;

    if (isSampleTable)
    {
        // compress the sample table as a single-precision int buffer
        //
        int32_t level = _get_zstd_level (encode);

        const int32_t compressedSize = _compress_ztsd_blosc_chunk (
            encode->packed_buffer,
            encode->packed_bytes,
            EXR_SINGLE_PRECISION_SIZE,
            encode->compressed_buffer,
            encode->compressed_bytes,
            level);

        if (compressedSize < 0)
            RETURN_ERROR (
                encode,
                EXR_ERR_COMPRESSION_FAILED,
                "[zstd]  Failed to compress sampleTable !!");

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
            RETURN_ERRORV (
                encode,
                rv,
                "[zstd]  Failed to allocate scratch buffer 1 (%" PRIu64
                " bytes)",
                encode->packed_bytes);

        rv = _new_encoding_zstd_data (encode);
        if (rv != EXR_ERR_SUCCESS) return rv;

        zstd_data_t* zstd_data = (zstd_data_t*) encode->encoding_user_data;

        // build the per-line cumulative sample table for this chunk.
        bool sampleCountTableIsCumulative =
            (encode->encode_flags &
             EXR_ENCODE_DATA_SAMPLE_COUNTS_ARE_INDIVIDUAL);

        _cumulative_samples_per_line (
            zstd_data,
            sampleCountTableIsCumulative,
            encode->sample_count_table);

        _unpack_channels (
            zstd_data,
            encode->packed_buffer,
            encode->packed_bytes,
            encode->scratch_buffer_1);
        assert (zstd_data->buffers[0] != NULL && zstd_data->buffers[1] != NULL);
        assert (
            zstd_data->buffers_size[0] + zstd_data->buffers_size[1] <=
            encode->packed_bytes);

#if (0)
        // validate unpacking / packing is a no-op
        rv = internal_encode_alloc_buffer (
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(encode->scratch_buffer_2),
            &(encode->scratch_alloc_size_2),
            encode->packed_bytes);
        _pack_channels (
            zstd_data,
            encode->scratch_buffer_1,
            encode->scratch_buffer_2,
            encode->packed_bytes);
        assert (
            memcmp (
                encode->packed_buffer,
                encode->scratch_buffer_2,
                encode->packed_bytes) == 0 &&
            "pack/unpack failed");
#endif

        rv = _compress_zstd (
            encode,
            encode->packed_bytes,
            encode->compressed_buffer,
            &(encode->compressed_bytes));

        if (rv ==
            EXR_ERR_COMPRESSION_FAILED /* || encode->compressed_bytes >= encode->packed_bytes */)
        {
            memcpy (
                encode->compressed_buffer,
                encode->packed_buffer,
                encode->packed_bytes);
            encode->compressed_bytes = encode->packed_bytes;
            rv                       = EXR_ERR_SUCCESS;
            DBGV (
                "internal_exr_apply_zstd: compression failed, comp = %zu, packed = %llu\n",
                encode->compressed_bytes,
                encode->packed_bytes);
        }
        assert (
            encode->compressed_bytes <= encode->packed_bytes &&
            "compressed size is too big");
    }

    return rv;
}

/**
 * \brief Decompression function called by C API.
 *
 * \param decode pointer to the decoding pipeline.
 * \param compressed_data pointer to compressed data buffer.
 * \param comp_buf_size size of compressed data buffer.
 * \param uncompressed_data pointer to uncompressed data buffer.
 * \param uncompressed_size expected size of uncompressed data buffer.
 * \return exr_result_t
 */
exr_result_t
internal_exr_undo_zstd (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    exr_result_t rv = EXR_ERR_SUCCESS;

    if (comp_buf_size == 0 || compressed_data == NULL)
    {
        decode->bytes_decompressed = 0;
        return EXR_ERR_SUCCESS;
    }

    bool isSampleTable = compressed_data == decode->sample_count_table;

    if (isSampleTable)
    {
        rv = _uncompress_zstd (
            decode,
            compressed_data,
            uncompressed_data,
            (int32_t) uncompressed_size,
            NULL);
        if (rv != EXR_ERR_SUCCESS) return rv;
    }
    else
    {
        // zstd didn't manage to shrink packed data and we stored packed data.
        if (comp_buf_size == uncompressed_size)
        {
            DBG ("internal_exr_undo_zstd: MEMCPY !\n")
            memcpy (uncompressed_data, compressed_data, uncompressed_size);
            decode->bytes_decompressed = uncompressed_size;
            return EXR_ERR_SUCCESS;
        }

        // we have an unpacked buffer to decompress.

        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(decode->scratch_buffer_1),
            &(decode->scratch_alloc_size_1),
            uncompressed_size);
        if (rv != EXR_ERR_SUCCESS)
        {
            ERROR_MSGV (
                decode,
                "[zstd]  Failed to allocate scratch buffer 1 (%" PRIu64
                " bytes)",
                uncompressed_size);
            return rv;
        }

        // setup our user data
        rv                     = _new_decoding_zstd_data (decode);
        zstd_data_t* zstd_data = (zstd_data_t*) decode->decoding_user_data;

        // compute expected sizes for validation
        _decoding_vars (decode);

        rv = _uncompress_zstd (
            decode,
            compressed_data,
            decode->scratch_buffer_1,
            uncompressed_size,
            zstd_data->buffers_size);
        if (rv != EXR_ERR_SUCCESS)
        {
            _free_decoding_zstd_data (decode);
            return rv;
        }

        _pack_channels (
            zstd_data,
            decode->scratch_buffer_1,
            uncompressed_data,
            uncompressed_size);

        _free_decoding_zstd_data (decode);
    }

    return EXR_ERR_SUCCESS;
}