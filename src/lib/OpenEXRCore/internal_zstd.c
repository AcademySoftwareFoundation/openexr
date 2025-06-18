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

static const uint64_t SERIALIZATION_OVERHEAD = sizeof(uint64_t);

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

    if (size+SERIALIZATION_OVERHEAD <= inSize && size + SERIALIZATION_OVERHEAD <= outPtrSize && size > 0)
    {
        //memcpy (outPtr, buffer, size);
        char* outPtrChar = (char*) outPtr;
        size = fn_serialize ((char*) buffer, size, outPtrChar, outPtrSize);
    }
    else
    {
        size = -1;
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
    const uint64_t*                   num_samples_per_row,
    int                               height,
    const exr_coding_channel_info_t* channels,
    int                              channelsSize,
    uint64_t*                        sorting_lookup)
{
    uint64_t writeCount = 0;
    uint64_t splitPoint = 0;
    {
        // First, count how many half channels we have
        for (int i = 0; i < channelsSize; ++i)
        {
            for (int h = 0; h < height; ++h)
            {
                const uint64_t numSamplesPerChunk = num_samples_per_row[h];
                if (channels[i].bytes_per_element == 2)
                {
                    *(sorting_lookup + h * channelsSize + i) = writeCount;
                    writeCount += numSamplesPerChunk * channels[i].bytes_per_element;
                }
            }
        }
    }
    splitPoint = writeCount; // This is where the half channels end
    {
        // First, count how many half channels we have
        for (int i = 0; i < channelsSize; ++i)
        {
            for (int h = 0; h < height; ++h)
            {
                const uint64_t numSamplesPerChunk = num_samples_per_row[h];
                if (channels[i].bytes_per_element == 4)
                {
                    *(sorting_lookup + h * channelsSize + i) = writeCount;
                    writeCount += numSamplesPerChunk * channels[i].bytes_per_element;
                }
            }
        }
    }

    return splitPoint; // Return the offset of the first float
}

// Reads H-F-H-F-F channels and, sorts to H-H-F-F-F returns the offset of the first float.
uint64_t
sort2_4ByteChannels_tiled (
    const char*                      inPtr,
    const uint64_t*                   num_samples_per_row,
    const exr_coding_channel_info_t* channels,
    const int                        channelsSize,
    const bool                       forward, // 1 = apply, 0 = un-apply
    int                              height,
    char*                            outPtr)
{
    uint64_t       sorting_lookup[channelsSize * height];

    uint64_t splitPoint  = compute_sorting_lookup (
            num_samples_per_row, height,channels, channelsSize, sorting_lookup);

    uint32_t pixel_stride = 0; // maybe already computed ?
    for (int i= 0; i < channelsSize; ++i)
    {
        pixel_stride += channels[i].bytes_per_element;
    }
    
    uint32_t processed_stride = 0;
    for (int i = 0; i < channelsSize; ++i)
    {
        uint64_t line_start_read = 0;
        for (int h = 0; h < height; ++h)
        {
            const uint64_t rowSortingLookup =
                *(sorting_lookup + h * channelsSize + i); 
            const uint64_t chan_start =
                num_samples_per_row[h] * processed_stride;

            const uint64_t writeIndex = rowSortingLookup;
            const uint64_t readIndex  = line_start_read + chan_start;
            const uint64_t bitSize = num_samples_per_row[h] * channels[i].bytes_per_element;
            if (forward) // apply or un-apply
            {
                memcpy (
                    outPtr + writeIndex,
                    inPtr + readIndex,
                    bitSize
                    );
            }
            else
            {
                memcpy (
                    outPtr + readIndex,
                    inPtr + writeIndex,
                    bitSize);
            }

            line_start_read += num_samples_per_row[h] * pixel_stride;
        }

        processed_stride += channels[i].bytes_per_element;
    }
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
_read_uint64 (char** src)
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
_write_uint64 (char** dst, const uint64_t bufByteSize)
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

static const uint64_t MAGIC_NUMBER = 8248453963162350458; // "zstd-exr"

uint64_t
serialize_buffer (char* src, uint64_t iSize, char* dest, uint64_t oSize)
{
    char* destPtr = dest;
    _write_uint64 (&destPtr, iSize);
    _write_buffer_data (&destPtr, iSize, src);
    return (uint64_t) (destPtr - dest); // return the number of bytes written
}

uint64_t
serialize_memcpy (char* src, uint64_t iSize, char* dest, uint64_t oSize)
{
    memcpy (dest, src, iSize);
    return iSize; // return the number of bytes written
}
bool
get_row_sample_count_decode (const exr_decode_pipeline_t* decode, uint64_t *row_sample_counts)
{
    int32_t*  sampleCountTable = decode->sample_count_table;
    const int sampleCountTableSize =
        decode->chunk.width *
        decode->chunk
            .height;

    if (decode->sample_count_valid == 1 && sampleCountTableSize > 0)
    {
        uint32_t readIdx = 0;
        uint64_t sampleCount = 0;
        for (int h = 0; h < decode->chunk.height; ++h)
        {
            if ((decode->decode_flags &
                 EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) == 1)
            {
                // cumulative samples
                row_sample_counts[h] =  
                    sampleCountTable[readIdx + decode->chunk.width -1]; // weird, might be width * height???
            }
            else
            {
                for (int i = readIdx; i < readIdx+decode->chunk.width; ++i)
                {
                    sampleCount += sampleCountTable[i];
                }
                row_sample_counts[h] =  sampleCount;
            }
            readIdx += decode->chunk.width; // overflow on last row ?
        }

        if ((decode->decode_flags & EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) == 0)
        {
            for (int h = 1; h < decode->chunk.height; ++h)
            {
                row_sample_counts[h] -=
                    row_sample_counts[h - 1]; // assumes monotonic increase
            }
        }

        return true; // we have a sample count table
    }
    return false; // we don't have a sample count table
}

bool
get_row_sample_count_encode (const exr_encode_pipeline_t* encode, uint64_t *row_sample_counts)
{
    int32_t*  sampleCountTable = encode->sample_count_table;
    const int sampleCountTableSize =
        encode->chunk.width *
        encode->chunk
            .height; //decode->sample_count_alloc_size / sizeof(int32_t); 

    if (encode->sample_count_alloc_size > 0 && sampleCountTableSize > 0)
    {
        uint32_t readIdx = 0;
        uint64_t sampleCount = 0;
        for (int h = 0; h < encode->chunk.height; ++h)
        {
            /*if ((encode->encode_flags &
                 EXR_DECODE_SAMPLE_COUNTS_AS_INDIVIDUAL) == 1) // seems flag is not set???*/
            {
                // cumulative samples
                row_sample_counts[h] =  
                    sampleCountTable[readIdx + encode->chunk.width -1]; // weird, might be width * height???
            }
            /*else
            {
                for (int i = readIdx; i < readIdx+encode->chunk.width; ++i)
                {
                    sampleCount += sampleCountTable[i];
                }
                row_sample_counts[h] =  sampleCount;
            }*/
            readIdx += encode->chunk.width; // overflow on last row ?
        }

        return true; // we have a sample count table
    }
    return false; // we don't have a sample count table
}

void
DebugChannelData (const exr_encode_pipeline_t* encode)
{
    uint8_t* inPtr        = (uint8_t*) encode->packed_buffer;
    int32_t*  sampleCountTable = encode->sample_count_table;
    const int sampleCountTableSize =
        encode->sample_count_alloc_size / sizeof (int32_t);
    const exr_coding_channel_info_t* channels     = encode->channels;
    const int                        channelsSize = encode->channel_count;

    bool tiled = (encode->chunk.type == EXR_STORAGE_DEEP_TILED);

    for (int s = encode->chunk.width-1; s < encode->chunk.width * encode->chunk.height; s+= encode->chunk.width)
    {
        uint32_t numSamples = sampleCountTable[s];
        int       readCount = 0;
        for (int i = 0; i < channelsSize; ++i)
        {
            float*    fdata;
            uint16_t* hdata;
            if (channels[i].bytes_per_element == 2)
            {
                hdata          = (uint16_t*) (inPtr + readCount);
                fdata          = (float*) (inPtr + readCount);
                uint16_t start = hdata[0];
                uint16_t end   = hdata[numSamples - 1];
                if (start != 14336 || end != 14336)
                {
                    printf ("Channel %hu: %s, start: %hu, end: %hu\n", i, channels[i].channel_name, start, end);
                }

            }
            else if (channels[i].bytes_per_element == 4)
            {
                fdata       = (float*) (inPtr + readCount);
                hdata       = NULL;
                float start = fdata[0];
                float end   = fdata[numSamples - 1];
                if (fabs(start - 1.10000002) > 1e5 || fabs(end - 1.10000002) > 1e5)
                {
                    printf ("Channel %hu: %s, start: %f, end: %f\n", i, channels[i].channel_name, start, end);
                }
            }
            else
            {
                fdata = NULL;
                hdata = NULL;
            }
            readCount += numSamples * channels[i].bytes_per_element;
        }
    }
}

bool needs_sorting( const exr_coding_channel_info_t* channels,
                    const int                        channelsSize,
                    const uint64_t*                  numSamples_per_row,
                    int height,
                    uint64_t*                        splitPosition,
                    uint8_t typeSizes[2])
{
    *splitPosition = 0;
    if (channelsSize <= 0)
    {
        return false; // no channels, nothing to sort
    }
    
    int8_t bytesPerChannel = channels[0].bytes_per_element; // if this is > 0, we have a float channel
    bool switched_bytes_per_channel = false;
    uint64_t writeCount =0;
    for (int i = 0; i < channelsSize; ++i)
    {
        for (int h = 0; h < height; ++h)
        {
            uint64_t numSamplesPerChunk = numSamples_per_row[h];
            if (channels[i].bytes_per_element != bytesPerChannel)
            {
                if (switched_bytes_per_channel)
                {
                    *splitPosition = 0;
                    return true; // we have already switched, so we need to sort
                }
                bytesPerChannel = channels[i].bytes_per_element;
                switched_bytes_per_channel = true;
                *splitPosition = writeCount;
            }
            writeCount += numSamplesPerChunk * channels[i].bytes_per_element;
        }
    }

    typeSizes[0] = channels[0].bytes_per_element;
    typeSizes[1] = channels[channelsSize - 1].bytes_per_element;
    return false;
}

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    uint64_t row_sample_counts[encode->chunk.height];
    bool sampleCount_valid = get_row_sample_count_encode (encode, row_sample_counts);
    const exr_coding_channel_info_t* channels     = encode->channels;
    const int                        channelsSize = encode->channel_count;
    exr_result_t                     rv           = EXR_ERR_SUCCESS;
    int32_t level;
    rv = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &level);

    if (rv != EXR_ERR_SUCCESS) return rv;

    int64_t compressedSize = 0;
    if (sampleCount_valid)
    {
        uint8_t* inPtr        = (uint8_t*) encode->packed_buffer;
        uint64_t inBufferSize = encode->packed_bytes;

        char*    outBuffer     = (char*) encode->compressed_buffer;
        uint64_t outBufferSize = encode->compressed_alloc_size;

        uint64_t splitPosition = 0;
        uint8_t typeSizes[2] = {0, 0}; // 0 = half, 1 = float
        bool needsSorting = encode->chunk.type == EXR_STORAGE_DEEP_TILED || needs_sorting (
            channels,
            channelsSize,
            row_sample_counts,
            encode->chunk.height,
            &splitPosition,
            typeSizes);
        char *compressionInBuffer = NULL;    
        if (needsSorting)
        {
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

            splitPosition = sort2_4ByteChannels_tiled (
                (const char*) inPtr,
                row_sample_counts,
                channels,
                channelsSize,
                true, // forward = true
                encode->chunk.height,
                (char*) encode->scratch_buffer_1);
            typeSizes[0] = 2;
            typeSizes[1] = 4;     
            compressionInBuffer = (char*) encode->scratch_buffer_1;
        }
        else
        {
            compressionInBuffer = (char*) inPtr;
        }

        bool bufferIsCompressed = true;;

        _write_uint64 (&outBuffer, MAGIC_NUMBER);
        outBufferSize -= sizeof (MAGIC_NUMBER);
        // we have 2 byte channels
        if (splitPosition > 0) // needed ?
        {
            inPtr        = (uint8_t *)compressionInBuffer;
            inBufferSize = splitPosition;

            long outSize = exr_compress_zstd (
                (char*)inPtr,
                inBufferSize,
                outBuffer,
                outBufferSize,
                typeSizes[0],
                level,
                &serialize_buffer);
            if (outSize < 0) { bufferIsCompressed = false; }

            compressedSize += outSize;
            outBuffer     = outBuffer + compressedSize;
            outBufferSize = outBufferSize - compressedSize;
            inPtr += inBufferSize;
            inBufferSize = encode->packed_bytes - splitPosition;
        }


        // if this points to the end, it means we have all half channels
        if (inBufferSize > 0 && bufferIsCompressed)
        {
            // we have 4 byte channels
            long outSize = exr_compress_zstd (
                (char*)inPtr,
                inBufferSize,
                outBuffer,
                outBufferSize,
                typeSizes[1],
                level,
                &serialize_buffer);
            if (outSize < 0) { bufferIsCompressed = false; }

            compressedSize += outSize;
        }
        if (!bufferIsCompressed)
        {
            // If we failed to compress, we just copy the data
            compressedSize = encode->packed_bytes;
            memcpy (encode->compressed_buffer, encode->packed_buffer, compressedSize);
        }
    }
    else
    {
        char* outPtr = (char*) encode->compressed_buffer;
        uint64_t outBufferSize = encode->compressed_alloc_size;
        _write_uint64 (&outPtr, MAGIC_NUMBER);
        outBufferSize -= sizeof (MAGIC_NUMBER);
        compressedSize = exr_compress_zstd (
            encode->packed_buffer,
            encode->packed_bytes,
            outPtr,
            outBufferSize,
            4,
            level,
            &serialize_memcpy);
        if (compressedSize < 0)
        {
            // If we failed to compress, we just copy the data
            compressedSize = encode->packed_bytes;
            memcpy (encode->compressed_buffer, encode->packed_buffer, compressedSize);
        }
        else
        {
            compressedSize += sizeof (MAGIC_NUMBER);
        }
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
    uint64_t row_sample_counts[decode->chunk.height];
    bool sampleCount_valid = get_row_sample_count_decode (decode, row_sample_counts);
    const exr_coding_channel_info_t* channels = decode->channels;
    const int                        channelsSize = decode->channel_count;
    exr_result_t   rv    = EXR_ERR_SUCCESS;

    char*    outPtr = NULL;
    uint64_t outBufferSize = 0;

    char*    inPtr = (char*) compressed_data;
    
    if (comp_buf_size < sizeof(uint64_t) ||  _read_uint64 (&inPtr) != MAGIC_NUMBER)
    {
        memcpy (uncompressed_data, compressed_data, comp_buf_size);
        return EXR_ERR_SUCCESS; // no zstd data, just copy the data
    }

    uint64_t inPtrSize = comp_buf_size - sizeof (MAGIC_NUMBER);

    if (sampleCount_valid)
    {
        uint64_t splitPosition = 0;
        uint8_t typeSizes[2] = {0, 0}; // 0 = half, 1 = float

        bool needsSorting = decode->chunk.type == EXR_STORAGE_DEEP_TILED ||
          needs_sorting (
            channels,
            channelsSize,
            row_sample_counts,
            decode->chunk.height,
            &splitPosition,
            typeSizes);
        if (needsSorting)
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
            outPtr = (char*) decode->scratch_buffer_1;
            outBufferSize    = decode->scratch_alloc_size_1;
        }
        else 
        { 
            outPtr = (char*) uncompressed_data; 
            outBufferSize    = uncompressed_size;
        }
 
        long     uncompressedSize = 0;
        {
            uint64_t compressedChannelChunkSize = _read_uint64 (&inPtr);
            if (compressedChannelChunkSize <= 0)
            {
                return EXR_ERR_CORRUPT_CHUNK;
            }

            long channelChunkUncompressedSize = exr_uncompress_zstd (
                (char*) inPtr,
                compressedChannelChunkSize,
                (void**)&outPtr,
                uncompressed_size);

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
                    _read_uint64 (&inPtr);
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

        if (needsSorting)
        {
            // Unshuffle the data
            uint64_t firstFloat = sort2_4ByteChannels_tiled (
                (char*) decode->scratch_buffer_1,
                row_sample_counts,
                channels,
                channelsSize,
                false, // backwards
                decode->chunk.height,
                (char*) uncompressed_data);
        }
    }
    else
    {
        long uncompressedSize = exr_uncompress_zstd (
            (char*) inPtr,
            inPtrSize,
            &uncompressed_data,
            uncompressed_size);
        if (uncompressed_size != uncompressedSize)
        {
            return EXR_ERR_CORRUPT_CHUNK;
        }
    }
    return rv;
}
