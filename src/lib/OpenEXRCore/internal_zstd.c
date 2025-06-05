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

#define RETURN_ERRORV(pipeline, err_code, msg, ...)                            \
    {                                                                          \
        exr_const_context_t pctxt = pipeline->context;                         \
        if (pctxt) pctxt->print_error (pctxt, err_code, msg, __VA_ARGS__);     \
        return err_code;                                                       \
    }

size_t
exr_get_zstd_lines_per_chunk ()
{
    return 1;
}

typedef uint64_t (*serialization_callback) (
    char* src, uint64_t iSize, char* dest, uint64_t oSize);

long
exr_compress_zstd (
    char*                  inPtr,
    int                    inSize,
    void*                  outPtr,
    int                    outPtrSize,
    uint64_t               typeSize,
    int32_t                level,
    serialization_callback fn_serialize)
{
    if (inSize == 0) // Weird input data when subsampling
    {
        outPtr = NULL;
        return 0;
    }

    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;

    cparams.typesize = typeSize;
    // clevel 9 is about a 20% increase in compression compared to 5.
    // Decompression speed is unchanged.
    cparams.clevel   = level;
    cparams.nthreads = 1;
    cparams.compcode = BLOSC_ZSTD; // Codec
    cparams.splitmode =
        BLOSC_NEVER_SPLIT; // Split => multithreading, not split better compression

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
        //memcpy (outPtr, buffer, size);
        char* outPtrChar = (char*) outPtr;
        size = fn_serialize ((char*) buffer, size, outPtrChar, outPtrSize);
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

long
exr_uncompress_zstd (
    char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    blosc2_schunk* _schunk =
        blosc2_schunk_from_buffer ((uint8_t*) inPtr, inSize, true);

    if (_schunk == NULL) { return -1; }

    if (outPtrSize == 0) // we don't have any storage allocated ? I don't think this will ever happen currently
    {
        *outPtr    = malloc (_schunk->nbytes);
        outPtrSize = _schunk->nbytes;
    }

    int size = blosc2_schunk_decompress_chunk (_schunk, 0, *outPtr, outPtrSize);
    blosc2_schunk_free (_schunk);

    return size;
}

uint64_t
compute_sorting_lookup (
    uint64_t                         numSamplesPerChunk,
    const exr_coding_channel_info_t* channels,
    int                              channelsSize,
    uint64_t*                        lookup)
{
    uint64_t writeCount = 0;
    uint64_t splitPoint = 0;
    {
        // First, count how many half channels we have
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element == 2)
            {
                lookup[i] = writeCount;
                writeCount += numSamplesPerChunk * 2;
            }
        }
    }
    splitPoint = writeCount; // This is where the half channels end
    {
        // Now, count how many float channels we have
        for (int i = 0; i < channelsSize; ++i)
        {
            if (channels[i].bytes_per_element == 4)
            {
                lookup[i] = writeCount;
                writeCount += numSamplesPerChunk * 4;
            }
        }
    }

    return splitPoint; // Return the offset of the first float
}

// Reads H-F-H-F-F channels and, sorts to H-H-F-F-F returns the offset of the first float.
uint64_t
sort2_4ByteChannels (
    const char*                      inPtr,
    const uint64_t                   numSamplesPerChunk,
    const exr_coding_channel_info_t* channels,
    const int                        channelsSize,
    const bool                       forward, // 1 = apply, 0 = un-apply
    char*                            outPtr)
{
    uint64_t       writeCount = 0;
    uint64_t       sorting_lookup[channelsSize];
    const uint64_t splitPoint = compute_sorting_lookup (
        numSamplesPerChunk, channels, channelsSize, sorting_lookup);

    uint64_t chanStart = 0;
    for (int i = 0; i < channelsSize; ++i)
    {
        if (forward) // apply or un-apply
        {
            memcpy (
                outPtr + sorting_lookup[i],
                inPtr + chanStart,
                numSamplesPerChunk * channels[i].bytes_per_element);
        }
        else
        {
            memcpy (
                outPtr + chanStart,
                inPtr + sorting_lookup[i],
                numSamplesPerChunk * channels[i].bytes_per_element);
        }
        chanStart += numSamplesPerChunk * channels[i].bytes_per_element;
    }

    /*{
        //first channel
        uint16_t* hdata = (uint16_t *)(outPtr);
        float* fdata = (uint16_t *)(outPtr + splitPoint); // fullshit
    }
    {
        // second channel
        float* fdata = (uint16_t *)(outPtr + splitPoint);
        float* hdata = (uint16_t *)(outPtr + splitPoint); // fullshit
    }
    {
        // second channel
        float* fdata = (uint16_t *)(outPtr + splitPoint + numSamplesPerChunk * 4);
        float* hdata = (uint16_t *)(outPtr + splitPoint); // fullshit
    }*/
    return splitPoint; // Return the offset of the first float
}

/**
 * \brief Read the buffer size from the buffer (4.29 GB max) and move
 * the read position to the start of buffer data.
 *
 * \param data  the buffer
 * \return      int32_t the buffer size in bytes
 */
static uint64_t
_read_buffer_size (char** src)
{
    uint64_t bufByteSize = 0;
    memcpy (&bufByteSize, *src, sizeof (bufByteSize));
    assert (bufByteSize <= UINT64_MAX && "read buffer size too large");
    *src += sizeof (bufByteSize); // move read position
    return bufByteSize;
}

/**
 * \brief Write the buffer size to the buffer (4.29 GB max) and move the write
 * position to the start of buffer data.
 *
 * \param data          the buffer
 * \param bufByteSize   the buffer size in bytes
 */
static void
_write_buffer_size (char** dst, const uint64_t bufByteSize)
{
    assert (bufByteSize <= UINT64_MAX && "write buffer size too large");
    memcpy (*dst, &bufByteSize, sizeof (bufByteSize));
    *dst += sizeof (bufByteSize); // move write position
}

static void
_write_buffer_data (char** dst, const int32_t bufByteSize, const void* src)
{
    assert (bufByteSize <= UINT64_MAX && "write data buffer size too large");
    if (bufByteSize > 0)
    {
        memcpy (*dst, src, bufByteSize);
        *dst += bufByteSize; // move write position
    }
}

uint64_t
serialize_buffer (char* src, uint64_t iSize, char* dest, uint64_t oSize)
{
    char* destPtr = dest;
    _write_buffer_size (&destPtr, iSize);
    _write_buffer_data (&destPtr, iSize, src);
    return (uint64_t) (destPtr - dest); // return the number of bytes written
}

uint64_t
serialize_memcpy (char* src, uint64_t iSize, char* dest, uint64_t oSize)
{
    memcpy (dest, src, iSize);
    return iSize; // return the number of bytes written
}
uint64_t
get_chunk_sample_count (const exr_decode_pipeline_t* decode)
{
    int32_t*  sampleCountTable = decode->sample_count_table;
    const int sampleCountTableSize =
        decode->chunk.width *
        decode->chunk
            .height; //decode->sample_count_alloc_size / sizeof(int32_t);

    if (decode->sample_count_valid == 1 && sampleCountTableSize > 0)
    {
        if ((decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) ==
            1)
        {
            // cumulative samples
            return sampleCountTable
                [sampleCountTableSize -
                 1]; // weird, might be -2 width * height???
        }
        else
        {
            uint64_t sampleCount = 0;
            for (int i = 0; i < sampleCountTableSize; ++i)
            {
                sampleCount += sampleCountTable[i];
            }
            return sampleCount;
        }
    }

    return 0; // no samples
}

void
DebugChannelData (
    const char*                      inPtr,
    int                              numSamples,
    const exr_coding_channel_info_t* channels,
    const int                        channelsSize)
{
    int readCount = 0;
    for (int i = 0; i < channelsSize; ++i)
    {
        float*    fdata;
        uint16_t* hdata;
        if (channels[i].bytes_per_element == 2)
        {
            hdata = (uint16_t*)(inPtr + readCount);
            fdata = NULL;
        }
        else if (channels[i].bytes_per_element == 4)
        {
            fdata = (float*)(inPtr + readCount);
            hdata = NULL;
        }
        else
        {
            fdata = NULL;
            hdata = NULL;
        }
        readCount += numSamples * channels[i].bytes_per_element;
    }
}

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    int32_t*  sampleCountTable = encode->sample_count_table;
    const int sampleCountTableSize =
        encode->sample_count_alloc_size / sizeof (int32_t);
    const exr_coding_channel_info_t* channels     = encode->channels;
    const int                        channelsSize = encode->channel_count;
    exr_result_t                     rv           = EXR_ERR_SUCCESS;
    int32_t level;
    rv = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &level);

    if (rv != EXR_ERR_SUCCESS) return rv;

    uint64_t compressedSize = 0;
    if (sampleCountTableSize > 0)
    {
        uint8_t* inPtr        = (uint8_t*) encode->packed_buffer;
        uint64_t inBufferSize = encode->packed_bytes;

        char*    outBuffer     = (char*) encode->compressed_buffer;
        uint64_t outBufferSize = encode->compressed_alloc_size;

        /*DebugChannelData (
            inPtr,
            sampleCountTable[sampleCountTableSize - 1],
            channels,
            channelsSize);*/
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

        uint64_t firstFloat = sort2_4ByteChannels (
            (const char*) inPtr,
            sampleCountTable
                [sampleCountTableSize -
                 1], // handle the non cumulative samples case
            channels,
            channelsSize,
            true, // forward = true
            (char*) encode->scratch_buffer_1);

        // we have 2 byte channels
        if (firstFloat > 0) // needed ?
        {
            inPtr        = (uint8_t*) encode->scratch_buffer_1;
            inBufferSize = firstFloat;
            long outSize = exr_compress_zstd (
                (char*)inPtr,
                inBufferSize,
                outBuffer,
                outBufferSize,
                2,
                level,
                &serialize_buffer);
            if (outSize < 0) { return EXR_ERR_UNKNOWN; }
            {
                /*char*    check     = encode->compressed_buffer;
                uint64_t checkSize = _read_buffer_size (&check);
                char     binaryChunk[4];
                memcpy (binaryChunk, check, 4);
                check += 4;*/
            }

            compressedSize += outSize;
            outBuffer     = outBuffer + compressedSize;
            outBufferSize = outBufferSize - compressedSize;
            inPtr += inBufferSize;
            inBufferSize = encode->packed_bytes - firstFloat;
        }

        // if this points to the end, it means we have all half channels
        if (inBufferSize > 0)
        {
            // we have 4 byte channels
            long outSize = exr_compress_zstd (
                (char*)inPtr,
                inBufferSize,
                outBuffer,
                outBufferSize,
                4,
                level,
                &serialize_buffer);
            if (outSize < 0) { return EXR_ERR_UNKNOWN; }

            {
                char*    check     = encode->compressed_buffer + compressedSize;
                uint64_t checkSize = _read_buffer_size (&check);
            }

            compressedSize += outSize;
        }
    }
    else
    {
        compressedSize = exr_compress_zstd (
            encode->packed_buffer,
            encode->packed_bytes,
            encode->compressed_buffer,
            encode->compressed_alloc_size,
            4,
            level,
            &serialize_memcpy);
    }

    if (compressedSize < 0) { return EXR_ERR_UNKNOWN; }

    encode->compressed_bytes = compressedSize;
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
    const uint64_t sampleCount                = get_chunk_sample_count (decode);
    const exr_coding_channel_info_t* channels = decode->channels;
    const int                        channelsSize = decode->channel_count;
    const uint8_t* inPtr = (const uint8_t*) decode->packed_buffer;
    exr_result_t   rv    = EXR_ERR_SUCCESS;

    if (sampleCount > 0)
    {
        rv = internal_decode_alloc_buffer (
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(decode->scratch_buffer_1),
            &(decode->scratch_alloc_size_1),
            uncompressed_size);

        if (rv != EXR_ERR_SUCCESS)
            RETURN_ERRORV (
                decode,
                rv,
                "[zstd]  Failed to allocate scratch buffer 1 (%" PRIu64
                " bytes)",
                uncompressed_size);

        char*    outPtr           = (char*) decode->scratch_buffer_1;
        char*    inPtr            = (char*) compressed_data;
        uint64_t outBufferSize    = decode->scratch_alloc_size_1;
        long     uncompressedSize = 0;
        {
            uint64_t compressedChannelChunkSize = _read_buffer_size (&inPtr);
            if (compressedChannelChunkSize <= 0)
            {
                return EXR_ERR_CORRUPT_CHUNK;
            }

            long channelChunkUncompressedSize = exr_uncompress_zstd (
                (char*) inPtr,
                compressedChannelChunkSize,
                (void**)&outPtr,
                uncompressed_size);

            if (channelChunkUncompressedSize <= 0)
            {
                return EXR_ERR_CORRUPT_CHUNK;
            }

            inPtr += compressedChannelChunkSize;
            outPtr += channelChunkUncompressedSize;
            outBufferSize -= channelChunkUncompressedSize;

            uncompressedSize += channelChunkUncompressedSize;
        }
        {
            // We have extra data
            if ((const void*)inPtr < compressed_data + comp_buf_size)
            {
                uint64_t compressedChannelChunkSize =
                    _read_buffer_size (&inPtr);
                if (compressedChannelChunkSize <= 0)
                {
                    return EXR_ERR_CORRUPT_CHUNK;
                }

                long channelChunkUncompressedSize = exr_uncompress_zstd (
                    (char*) inPtr,
                    compressedChannelChunkSize,
                    (void**)&outPtr,
                    outBufferSize);

                if (channelChunkUncompressedSize <= 0)
                {
                    return EXR_ERR_CORRUPT_CHUNK;
                }

                uncompressedSize += channelChunkUncompressedSize;
            }
        }

        if (uncompressed_size != uncompressedSize)
        {
            return EXR_ERR_CORRUPT_CHUNK;
        }

        // Unshuffle the data
        uint64_t firstFloat = sort2_4ByteChannels (
            (char*) decode->scratch_buffer_1,
            sampleCount,
            channels,
            channelsSize,
            false, // backwards
            (char*) uncompressed_data);

        //DebugChannelData (
        //    uncompressed_data, sampleCount, channels, channelsSize);
    }
    else
    {
        long uncompressedSize = exr_uncompress_zstd (
            (char*) compressed_data,
            comp_buf_size,
            &uncompressed_data,
            uncompressed_size);
        if (uncompressed_size != uncompressedSize)
        {
            return EXR_ERR_CORRUPT_CHUNK;
        }
    }
    return rv;
}
