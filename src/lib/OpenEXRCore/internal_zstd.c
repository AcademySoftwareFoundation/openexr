/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr_compression.h>
#include "internal_compress.h"
#include "internal_decompress.h"
#include "blosc2.h"

size_t
exr_get_zstd_lines_per_chunk ()
{
    return 1;
}

static long
compress_ztsd_blosc_chunk (
    char* inPtr, int inSize, int typeSize, void* outPtr, int outPtrSize)
{
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize       = typeSize;
    // clevel 9 is about a 20% increase in compression compared to 5.
    // Decompression speed is unchanged.
    int zstd_level;
    exr_get_default_zstd_compression_level (&zstd_level);
    cparams.clevel   = zstd_level;
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

static long
uncompress_ztsd_blosc_chunk (
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    blosc2_schunk* _schunk =
        blosc2_schunk_from_buffer ((uint8_t*) inPtr, inSize, true);
    if (_schunk == NULL) { return -1; }

    if (outPtrSize == 0) // we don't have any storage allocated
    {
        *outPtr    = malloc (_schunk->nbytes);
        outPtrSize = _schunk->nbytes;
    }

    int size = blosc2_schunk_decompress_chunk (_schunk, 0, *outPtr, outPtrSize);
    blosc2_schunk_free (_schunk);

    return size;
}

long
exr_compress_zstd (
    char*  inPtr,
    int    inSize,
    int    numSamples,
    int*   channelTypeSizes,
    size_t channelSizesCount,
    void*  outPtr,
    int    outPtrSize)
{
    const int magicNumber =
        0xDEADBEEF01; // did we compress the stream or just copied the input

    if (inSize == 0) // Weird input data when subsampling
    {
        outPtr = NULL;
        return 0;
    }

    // We are given as input every channel typesize and number of samples.
    // All contiguous channels that share the same typesize are batched into
    // the same call to Blosc
    long numChunks = 0;
    int  chunkTypeSize[channelSizesCount]; // typesize to feed to Blosc
    long chunkSizes[channelSizesCount];    // where the chunk starts

    memset (chunkTypeSize, 0, sizeof (chunkTypeSize));
    memset (chunkSizes, 0, sizeof (chunkSizes));

    chunkTypeSize[numChunks] = channelTypeSizes[numChunks];
    chunkSizes[numChunks]    = chunkTypeSize[numChunks] * numSamples;
    for (int i = 1; i < channelSizesCount; ++i)
    {
        if (chunkTypeSize[numChunks] != channelTypeSizes[i])
        {
            numChunks++;
            chunkTypeSize[numChunks] = channelTypeSizes[i];
            chunkSizes[numChunks]    = chunkTypeSize[numChunks] * numSamples;
        }
        else { chunkSizes[numChunks] += chunkTypeSize[numChunks] * numSamples; }
    }

    numChunks++;

    char* inputPointer        = inPtr;  // tracks the reading pointer
    char* outputPointer       = outPtr; // tracks the output
    long  totalCompressedSize = 0;      // tracks the total amount of bytes

    // for crazy 4 bytes inputs we want to copy the data as-is
    if (totalCompressedSize + sizeof (numChunks) > inSize)
    {
        memcpy (outPtr, inPtr, inSize);
        return inSize;
    }

    memcpy (outputPointer, &numChunks, sizeof (numChunks));
    outputPointer += sizeof (numChunks);
    totalCompressedSize += sizeof (numChunks);

    bool copyInputAsIs = false; // in a rare case where the input data is
                                // white noise, we would actually grow the size
                                // by compressing.
    void* scratch = malloc (inSize);
    for (int i = 0; i < numChunks; ++i)
    {
        long compressedChunkSize = compress_ztsd_blosc_chunk (
            inputPointer, chunkSizes[i], chunkTypeSize[i], scratch, inSize);
        if (totalCompressedSize + compressedChunkSize +
                sizeof (compressedChunkSize) >
            inSize)
        {
            // if appending this compressed chunk would yield a larger file,
            // abandon
            copyInputAsIs = true;
            break;
        }

        // write the amount of bytes of the chunk
        memcpy (
            outputPointer, &compressedChunkSize, sizeof (compressedChunkSize));
        outputPointer += sizeof (compressedChunkSize);
        totalCompressedSize += compressedChunkSize;

        // write the chunk data
        memcpy (outputPointer, scratch, compressedChunkSize);
        outputPointer += compressedChunkSize;
        totalCompressedSize += sizeof (compressedChunkSize);

        inputPointer += chunkSizes[i];
    }

    free (scratch);

    if (copyInputAsIs)
    {
        memcpy (outPtr, inPtr, inSize);
        return inSize;
    }

    return totalCompressedSize;
}

long
exr_uncompress_zstd (
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    long  numChunks;
    char* inputPointer          = inPtr; // tracks the reading pointer
    long  totalDecompressedSize = 0;

    memcpy (&numChunks, inputPointer, sizeof (numChunks));
    inputPointer += sizeof (numChunks);

    bool copyInputAsIs = false;

    char* scratch[numChunks];
    long  scratchSize[numChunks];

    for (int i = 0; i < numChunks; ++i)
    {
        long chunkSize;
        memcpy (&chunkSize, inputPointer, sizeof (chunkSize));
        inputPointer += sizeof (chunkSize);

        void* out;
        // this will allocate "scratch" since we pass 0
        const long decompressedChunkSize =
            uncompress_ztsd_blosc_chunk (inputPointer, chunkSize, &out, 0);
        scratch[i] = out;
        if (decompressedChunkSize == -1)
        {
            // blosc failed to decompress. probably because the stream was not
            // compressed to start with
            copyInputAsIs = true;
            break;
        }

        //memcpy (outputPointer, scratch, decompressedChunkSize);

        inputPointer += chunkSize;
        scratchSize[i] = decompressedChunkSize;
        totalDecompressedSize += decompressedChunkSize;
    }

    if (copyInputAsIs)
    {
        if (outPtrSize == 0) { *outPtr = malloc (inSize); }

        memcpy (*outPtr, inPtr, inSize);
        return inSize;
    }

    if (outPtrSize == 0) { *outPtr = malloc (totalDecompressedSize); }

    char* outputPointer = *outPtr;

    for (int i = 0; i < numChunks; ++i)
    {
        memcpy (outputPointer, scratch[i], scratchSize[i]);
        outputPointer += scratchSize[i];
    }

    for (int i = 0; i < numChunks; ++i)
    {
        free (scratch[i]);
    }

    return totalDecompressedSize;
}

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    int channelSizes[encode->channel_count];
    int channelSizesCount = encode->channel_count;
    int totalChannelSize  = 0;
    for (int i = 0; i < encode->channel_count; ++i)
    {
        channelSizes[i] = (int) encode->channels[i].bytes_per_element;
        totalChannelSize += (int) encode->channels[i].bytes_per_element;
    }

    const int numSamples = encode->packed_bytes / totalChannelSize;

    if (numSamples * totalChannelSize != encode->packed_bytes)
    {
        channelSizesCount = 1;
        // We received fewer data than expected. It probably is because we are processing
        // the sampleCounts for DeepExr
        channelSizes[0] = 4; // we compress it as an int
    }

    long compressedSize = exr_compress_zstd (
        encode->packed_buffer,
        encode->packed_bytes,
        numSamples,
        channelSizes,
        channelSizesCount,
        encode->compressed_buffer,
        encode->compressed_alloc_size);
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

    long uncompressedSize = exr_uncompress_zstd (
        compressed_data, comp_buf_size, &uncompressed_data, uncompressed_size);
    if (uncompressed_size != uncompressedSize) { return EXR_ERR_CORRUPT_CHUNK; }
    return EXR_ERR_SUCCESS;
}