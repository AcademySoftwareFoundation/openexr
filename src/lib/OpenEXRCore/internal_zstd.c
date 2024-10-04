/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr_compression.h>
#include "internal_compress.h"
#include "internal_decompress.h"
#include <assert.h>
#include "blosc2.h"

static const size_t EXR_HALF_PRECISION_SIZE   = 2;
static const size_t EXR_SINGLE_PRECISION_SIZE = 4;

size_t
exr_get_zstd_lines_per_chunk ()
{
    return 1;
}

static long
compress_ztsd_blosc_chunk (
    char* inPtr,
    int   inSize,
    int   typeSize,
    void* outPtr,
    int   outPtrSize,
    int   zstd_level)
{
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize       = typeSize;
    // clevel 9 is about a 20% increase in compression compared to 5.
    // Decompression speed is unchanged.
    cparams.clevel    = zstd_level;
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
    int    outPtrSize,
    int    zstd_level)
{
    // FIXME: clang warns about this magicNumber:
    // warning: implicit conversion from 'long' to 'int' changes value from 956397711105 to -1379995903
    // I don't know what this is for: it's not used anywhere.
    // const int magicNumber =
    //     0xDEADBEEF01; // did we compress the stream or just copied the input

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
            inputPointer,
            chunkSizes[i],
            chunkTypeSize[i],
            scratch,
            inSize,
            zstd_level);
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

void
cumulative_samples_per_line (
    int        lineCount,          // in: number of lines
    const int* sampleCountPerLine, // in: number of samples per line
    int*       cumSampsPerLine     // out: cumulative samples per lines
)
{
    cumSampsPerLine[0] = 0;
    for (int i = 0; i < lineCount; ++i)
    {
        cumSampsPerLine[i + 1] = cumSampsPerLine[i] + sampleCountPerLine[i];
    }
}

// returns start offsets for all planar channels,
//
void
channel_offsets (
    int        channelCount,     // in: nuber of channels
    const int* channelsTypeSize, // in: type byte size per channel
    int        bufSampleCount,   // in: total number of samples in buffer
    size_t*    chOffsets,        // out: buffer offsets array
    size_t*    typeSplitOfst     // out: offset of single precison data
)
{
    // count the number of half and single precision channels
    int n_half   = 0;
    int n_single = 0;
    for (int ch = 0; ch < channelCount; ++ch)
    {
        if (channelsTypeSize[ch] == EXR_HALF_PRECISION_SIZE)
            ++n_half;
        else
            ++n_single;
    }

    // map offsets to channel numbers
    int    nh           = 0;
    int    ns           = 0;
    size_t half_ch_size = bufSampleCount * EXR_HALF_PRECISION_SIZE;
    size_t out_split    = n_half * half_ch_size;
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
    *typeSplitOfst = out_split;
}

// Unpack a scanline/tile buffer into a single buffer. Half channels come first,
// followed by float/uint channels). outSplitPos marks the begining of float/uint
// data.
// The buffers contain per-channel planar (multi-line) data.
// Supports deep files by handling arbitrary number of samples per pixel.
//
// Example: 2 lines of 3 pixels with half r, float g, half b, uint i channels:
//
// before:
// [rh rh rh gs gs gs bh bh bh is is is rh rh rh gs gs gs bh bh bh is is is]
//
// after:
// [rh rh rh rh rh rh bh bh bh bh bh bh gs gs gs gs gs gs is is is is is is]
// ^                                   ^
// outPos                          outSplitPos
//
void
unpack_channels (
    const char*            inPtr,              // input data
    const int              inSize,             // input data size
    const exr_attr_box2i_t range,              // single/multi scanline or tile
    const int              channelCount,       // number of input channels
    const int*             channelsTypeSize,   // type sizes for each channel
    const int*             sampleCountPerLine, // samples count per line
    char*                  outPtr,      // out: pre-allocated buffer pointer
    int*                   outSplitOfst // out: pointer to single precision data
)
{
    int lineCount = range.max.y - range.min.y + 1;

    int cumSampsPerLine[lineCount + 1];
    cumulative_samples_per_line (
        lineCount, sampleCountPerLine, cumSampsPerLine);
    int bufSampleCount = cumSampsPerLine[lineCount];

    size_t chOffsets[channelCount];
    size_t splitOffset = 0;
    channel_offsets (
        channelCount,
        channelsTypeSize,
        bufSampleCount,
        chOffsets,
        &splitOffset);

    char* inPos = (char*) inPtr;
    for (int ln = 0; ln < lineCount; ++ln)
    {
        for (int ch = 0; ch < channelCount; ++ch)
        {
            size_t copySize = channelsTypeSize[ch] * sampleCountPerLine[ln];
            char*  outPos   = outPtr + chOffsets[ch] +
                           cumSampsPerLine[ln] * channelsTypeSize[ch];
            memcpy (outPos, inPos, copySize);
            inPos += copySize;
        }
    }

    *outSplitOfst = (int) splitOffset;
}

// new implementation - philippe
//
// output buffer layout:
//  [
//      size_t half_buffer_size
//      half_data
//      ...
//      size_t single_buffer_size
//      single_data
//      ...
//  ]
//
long
exr_compress_zstd_v2 (
    const char*            inPtr,              // input buffer
    const size_t           inSize,             // input buffer size
    const exr_attr_box2i_t range,              // scanline(s) / tile bounds
    const int              channelCount,       // number of channels
    const int*             channelsTypeSize,   // byte size per channel
    const int*             sampleCountPerLine, // number of samples per line
    void*                  outPtr,             // output buffer
    int                    zstd_level          // compression level
)
{
    // case where stride > 1 and we should skip.
    if (inSize == 0)
    {
        outPtr = NULL;
        return 0;
    }

    // FIXME: outPtr is not allocated !!
    char* flatBuf         = NULL; // storage for unpacked channels
    char* flatBufSplitPos = NULL; // pointer to full precision data section

    if (sampleCountPerLine[0] == 0)
    {
        // compress sample table: use the single precision buffer.
        flatBuf         = (char*) inPtr;
        flatBufSplitPos = (char*) inPtr;
    }
    else
    {
        flatBuf         = malloc (inSize);
        int flatBufOfst = 0;
        unpack_channels (
            inPtr,
            inSize,
            range,
            channelCount,
            channelsTypeSize,
            sampleCountPerLine,
            flatBuf,
            &flatBufOfst);
        flatBufSplitPos = flatBuf + flatBufOfst;
    }
    assert (flatBufSplitPos != NULL);

    // compress buffers here
    long  outPtrSize  = 0;
    void* outPtrPos   = outPtr;
    void* scratch     = malloc (inSize);
    char* bufs[2]     = {flatBuf, flatBufSplitPos};
    long  bufsSize[2] = {
        flatBufSplitPos > flatBuf ? flatBufSplitPos - flatBuf : 0,
        (flatBuf + inSize) - flatBufSplitPos};
    int bufsDataSize[2] = {EXR_HALF_PRECISION_SIZE, EXR_SINGLE_PRECISION_SIZE};
    for (unsigned b = 0; b < 2; ++b)
    {
        // compress buffer section
        long compressedSize = 0;
        if (bufsSize[b] > 0)
        {
            compressedSize = compress_ztsd_blosc_chunk (
                bufs[b],
                bufsSize[b],
                bufsDataSize[b],
                scratch,
                inSize,
                zstd_level);
            if (sampleCountPerLine[0] == 0)
                printf ("  > table size = %d\n", (int) compressedSize);
            else
                printf ("  > buf size = %d\n", (int) compressedSize);
        }

        // always write buffer size
        bool   isSmaller = compressedSize < bufsSize[b];
        size_t outSize   = isSmaller ? (size_t) compressedSize
                                     : (size_t) bufsSize[b];
        memcpy (outPtrPos, &outSize, sizeof (size_t));
        outPtrPos += sizeof (outSize);
        outPtrSize += sizeof (outSize); // need to add sizes to the output
                                        // stream length
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
    if (flatBuf != inPtr) free (flatBuf);

    return outPtrSize;
}

long
exr_uncompress_zstd (
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    long  numChunks;
    char* inputPointer          = (char*) inPtr; // tracks the reading pointer
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

// pack an unpacked buffer into a scanline/tile buffer. Half channels come first,
// followed by float/uint channels). outSplitPos marks the begining of float/uint
// data.
// The input buffers contain per-channel planar (multi-line) data.
// Supports deep files by handling arbitrary number of samples per pixel.
//
// Example
//  2 lines of 3 pixels with half r, float g, half b, uint i channels, 1 sample
//  per pixel (non-deep file).
//
//  before:
//  [rh rh rh rh rh rh bh bh bh bh bh bh gs gs gs gs gs gs is is is is is is]
//
//  after:
//  [rh rh rh gs gs gs bh bh bh is is is rh rh rh gs gs gs bh bh bh is is is]
//
void
pack_channels (
    const char* inPtr,
    const int   inSize,
    const int   channelCount,
    const int*  channelsTypeSize,
    const int   lineCount,
    const int*  sampleCountPerLine,
    void*       outPtr,
    const int   outPtrByteSize)
{
    int cumSampsPerLine[lineCount + 1];
    cumulative_samples_per_line (
        lineCount, sampleCountPerLine, cumSampsPerLine);
    int bufSampleCount = cumSampsPerLine[lineCount];

    size_t chOffsets[channelCount];
    size_t splitOffset = 0;
    channel_offsets (
        channelCount,
        channelsTypeSize,
        bufSampleCount,
        chOffsets,
        &splitOffset);

    char* outPos        = (char*) outPtr;
    int   totalByteSize = 0;
    for (int ln = 0; ln < lineCount; ++ln)
    {
        for (int ch = 0; ch < channelCount; ++ch)
        {
            size_t copySize = channelsTypeSize[ch] * sampleCountPerLine[ln];
            int    cts      = channelsTypeSize[ch];
            char*  inPos    = (char*) inPtr + chOffsets[ch] +
                          cumSampsPerLine[ln] * channelsTypeSize[ch];
            memcpy (outPos, inPos, copySize);
            outPos += copySize;
            // debug
            totalByteSize += copySize;
            assert (totalByteSize <= outPtrByteSize);
        }
    }
}

long
exr_uncompress_zstd_v2 (
    const char*    inPtr,
    const uint64_t inSize,
    const int      channelCount,
    const int*     channelsTypeSize,
    const int      lineCount,
    const int*     sampleCountPerLine,
    char*          outPtr,
    const int      outPtrByteSize)
{
    int   outSize        = 0;
    char* inPtrPos       = (char*) inPtr;
    int   decompSize     = outPtrByteSize;
    char* decompPtr      = malloc (decompSize);
    char* decompWritePos = decompPtr;
    char* decompPtrs[2];

    for (int b = 0; b < 2; ++b)
    {
        // read compressed buffer size
        size_t compressedBufSize = 0;
        memcpy (&compressedBufSize, inPtrPos, sizeof (compressedBufSize));
        inPtrPos += sizeof (compressedBufSize); // move read position
        decompPtrs[b] = inPtrPos;

        if (compressedBufSize == 0) continue;

        // read buffer
        const long decompressedSize = uncompress_ztsd_blosc_chunk (
            inPtrPos,
            compressedBufSize,
            (void**) &decompWritePos,
            decompSize // ask function to allocate required memory
        );
        if (decompressedSize < 0) printf ("ERROR: bloc2 failed to compress !!");
        outSize += decompressedSize;
        inPtrPos += compressedBufSize;
        decompWritePos += decompressedSize;
    }

    if (sampleCountPerLine[0] == 0)
    {
        // we decompressed the sample count table
        memcpy (outPtr, decompPtrs[1], outSize);
        return outSize;
    }

    pack_channels (
        decompPtr,
        inSize,
        channelCount,
        channelsTypeSize,
        lineCount,
        sampleCountPerLine,
        outPtr,
        outPtrByteSize);

    return outSize;
}

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    // Get the compression level from the context
    int          level = 5; // default compression level
    exr_result_t rv;
    rv = exr_get_zstd_compression_level (
        encode->context, encode->part_index, &level);
    if (rv != EXR_ERR_SUCCESS) return rv;

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
        encode->compressed_alloc_size,
        level);
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