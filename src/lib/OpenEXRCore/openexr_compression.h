/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_COMPRESSION_H
#define OPENEXR_CORE_COMPRESSION_H

#include "openexr_context.h"
#include "openexr_attr.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file */

/** Computes a buffer that will be large enough to hold the compressed
 * data. This may include some extra padding for headers / scratch */
EXR_EXPORT
size_t exr_compress_max_buffer_size (size_t in_bytes);

/** Compresses a buffer using a zlib style compression.
 *
 * If the level is -1, will use the default compression set to the library
 * \ref exr_set_default_zip_compression_level
 * data. This may include some extra padding for headers / scratch */
EXR_EXPORT
exr_result_t exr_compress_buffer (
    exr_const_context_t ctxt,
    int                 level,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out);

EXR_EXPORT
exr_result_t exr_uncompress_buffer (
    exr_const_context_t ctxt,
    const void*         in,
    size_t              in_bytes,
    void*               out,
    size_t              out_bytes_avail,
    size_t*             actual_out);

EXR_EXPORT
long exr_compress_zstd (
    char*  inPtr,
    int    inSize,
    int    numSamples,
    int*   channelTypeSizes,
    size_t channelSizesCount,
    void*  outPtr,
    int    outPtrSize);

EXR_EXPORT
long exr_compress_zstd_v2 (
    const char*            inPtr,
    const size_t           inSize,
    const exr_attr_box2i_t range,
    const int              channelCount,
    const int*             channelsTypeSize,
    const int*             sampleCountPerLine,
    void*                  outPtr);

EXR_EXPORT
long exr_uncompress_zstd (
    const char* inPtr, uint64_t inSize, void** outPtr, uint64_t outPtrSize);

EXR_EXPORT
long exr_uncompress_zstd_v2 (
    const char*    inPtr,
    const uint64_t inSize,
    const int      channelCount,
    const int*     channelsTypeSize,
    const int      lineCount,
    const int*     sampleCountPerLine,
    char*          outPtr);

EXR_EXPORT
size_t exr_get_zstd_lines_per_chunk ();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_COMPRESSION_H */
