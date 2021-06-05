/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_CHUNKIO_H
#define OPENEXR_CORE_CHUNKIO_H

#include "openexr_part.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure describing raw data information about a chunk
 */
typedef struct
{
    int32_t idx;

    int32_t start_x;
    int32_t start_y;
    int32_t height; /**< for this chunk */
    int32_t width;  /**< for this chunk */

    uint8_t level_x; /**< for tiled files */
    uint8_t level_y; /**< for tiled files */

    uint8_t type;
    uint8_t compression;

    uint64_t data_offset;
    uint64_t packed_size;
    uint64_t unpacked_size;

    uint64_t sample_count_data_offset;
    uint64_t sample_count_table_size;
} exr_chunk_block_info_t;

EXR_EXPORT
exr_result_t exr_read_scanline_block_info (
    exr_const_context_t     ctxt,
    int                     part_index,
    int                     y,
    exr_chunk_block_info_t* cinfo);

EXR_EXPORT
exr_result_t exr_read_tile_block_info (
    exr_const_context_t     ctxt,
    int                     part_index,
    int                     tilex,
    int                     tiley,
    int                     levelx,
    int                     levely,
    exr_chunk_block_info_t* cinfo);

/** Read the packed data block for a chunk
 *
 * This assumes that the buffer pointed to by @param packed_data is
 * large enough to hold the chunk block info packed_size bytes
 */
EXR_EXPORT
exr_result_t exr_read_chunk (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    void*                         packed_data);

/**
 * Read chunk for deep data.
 *
 * allows one to read the packed data, the sample count data, or both.
 * @sa exr_read_chunk also works to read deep data packed data,
 * but this is a routine to get the sample count table and the packed
 * data in one go, or if you want to pre-read the sample count data,
 * you can get just that buffer.
 */
EXR_EXPORT
exr_result_t exr_read_deep_chunk (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    void*                         packed_data,
    void*                         sample_data);

/** y must the appropriate starting y for the specified chunk */
EXR_EXPORT
exr_result_t exr_write_scanline_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           y,
    const void*   packed_data,
    uint64_t      packed_size);

/** y must the appropriate starting y for the specified chunk */
EXR_EXPORT
exr_result_t exr_write_deep_scanline_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           y,
    const void*   packed_data,
    uint64_t      packed_size,
    uint64_t      unpacked_size,
    const void*   sample_data,
    uint64_t      sample_data_size);

EXR_EXPORT
exr_result_t exr_write_tile_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           tilex,
    int           tiley,
    int           levelx,
    int           levely,
    const void*   packed_data,
    uint64_t      packed_size);

EXR_EXPORT
exr_result_t exr_write_deep_tile_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           tilex,
    int           tiley,
    int           levelx,
    int           levely,
    const void*   packed_data,
    uint64_t      packed_size,
    uint64_t      unpacked_size,
    const void*   sample_data,
    uint64_t      sample_data_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_CHUNKIO_H */
