/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_LEGACY_STRUCTS_H
#define OPENEXR_CORE_LEGACY_STRUCTS_H

#include "openexr_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _exr_decode_pipeline_v1
{
    size_t pipe_size;

    exr_coding_channel_info_t* channels;
    int16_t                    channel_count;
    uint16_t decode_flags;
    int                 part_index;
    exr_const_context_t context;
    exr_chunk_info_t    chunk;
    int32_t user_line_begin_skip;
    int32_t user_line_end_ignore;
    uint64_t bytes_decompressed;
    void* decoding_user_data;
    void* packed_buffer;
    size_t packed_alloc_size;
    void* unpacked_buffer;
    size_t unpacked_alloc_size;
    void*  packed_sample_count_table;
    size_t packed_sample_count_alloc_size;
    int32_t* sample_count_table;
    size_t   sample_count_alloc_size;
    void* scratch_buffer_1;
    size_t scratch_alloc_size_1;
    void* scratch_buffer_2;
    size_t scratch_alloc_size_2;

    void* (*alloc_fn) (exr_transcoding_pipeline_buffer_id_t, size_t);
    void (*free_fn) (exr_transcoding_pipeline_buffer_id_t, void*);
    exr_result_t (*read_fn) (struct _exr_decode_pipeline* pipeline);
    exr_result_t (*decompress_fn) (struct _exr_decode_pipeline* pipeline);
    exr_result_t (*realloc_nonimage_data_fn) (
        struct _exr_decode_pipeline* pipeline);
    exr_result_t (*unpack_and_convert_fn) (
        struct _exr_decode_pipeline* pipeline);
    exr_coding_channel_info_t _quick_chan_store[5];
} exr_decode_pipeline_v1_t;

/* change this if we add additional members in the future */
typedef struct _exr_decode_pipeline exr_decode_pipeline_v2_t;

#ifdef __cplusplus
}
#endif

#endif /* OPENEXR_CORE_DECOMPRESS_H */
