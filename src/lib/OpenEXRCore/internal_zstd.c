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

long
exr_compress_zstd (char* inPtr, int inSize, void* outPtr, int outPtrSize)
{
    if (inSize == 0) // Weird input data when subsampling
    {
        outPtr = NULL;
        return 0;
    }

    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    int typeSize     = inSize % 4 == 0 ? 4 : 2;
    cparams.typesize = typeSize;
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
    { memcpy (outPtr, buffer, size); }
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
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize)
{
    blosc2_schunk* _schunk = blosc2_schunk_from_buffer ((uint8_t *)inPtr, inSize, true);

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

exr_result_t
internal_exr_apply_zstd (exr_encode_pipeline_t* encode)
{
    long compressedSize = exr_compress_zstd (
        encode->packed_buffer,
        encode->packed_bytes,
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
        (const char*) compressed_data,
        comp_buf_size,
        &uncompressed_data,
        uncompressed_size);
    if (uncompressed_size != uncompressedSize) { return EXR_ERR_CORRUPT_CHUNK; }
    return EXR_ERR_SUCCESS;
}