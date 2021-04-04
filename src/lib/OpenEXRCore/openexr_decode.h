/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_DECODE_H
#define OPENEXR_CORE_DECODE_H

#include "openexr_chunkio.h"
#include "openexr_coding.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure meant to be used on a per-thread basis for reading exr data
 *
 * As should be obvious, this structure is NOT thread safe, but rather
 * meant to be used by separate threads, which can all be accessing
 * the same context concurrently.
 */
typedef struct _exr_decode_pipeline
{
    /** the output channel information for this chunk
     *
     * User is expected to fill the channel pointers for the desired
     * output channels (any that are NULL will be skipped) if you are
     * going to use @sa exr_choose_unpack_routine. If all that is
     * desired is to read and decompress the data, this can be left
     * uninitialized.
     *
     * Describes the channel information. This information is
     * allocated dynamically during @sa exr_initialize_decoding
     */
    exr_coding_channel_info_t* channels;
    int16_t                    channel_count;

    /**
     * can be used by the user to pass custom context data through
     * the decode pipeline
     */
    void* decoding_user_data;

    /** copy of the parameters given to the initialize / update for convenience */
    exr_context_t          context;
    int                    part_index;
    exr_chunk_block_info_t chunk_block;

    /** the (compressed) buffer.
     *
     * If null, will be allocated during the run of the pipeline.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* packed_buffer;
    /** used when re-using the same decode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     */
    size_t packed_alloc_size;
    /** the decompressed buffer (unpacked_size from the chunk block
     * info), but still packed into storage order, only needed for
     * compressed files
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* unpacked_buffer;
    /** used when re-using the same decode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     */
    size_t unpacked_alloc_size;
    /** a scratch buffer of unpacked_size for intermediate results
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* scratch_buffer_1;
    /** used when re-using the same decode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     */
    size_t scratch_alloc_size_1;
    /** some decompression routines may need a second scratch buffer (i.e. zlib)
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* scratch_buffer_2;
    /** used when re-using the same decode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough */
    size_t scratch_alloc_size_2;

    /** for deep data */
    void* packed_sample_count_table;
    /** for deep data */
    size_t packed_sample_count_alloc_size;
    /** for deep data */
    int32_t* sample_count_table;
    /** for deep data */
    size_t sample_count_alloc_size;
    
    /**
     * enables a custom allocator for the different buffers (i.e. if
     * decoding on a GPU). If NULL, will use the allocator from the
     * context
     */
    void* (*alloc_fn) (enum transcoding_pipeline_buffer_id, size_t);
    /**
     * enables a custom allocator for the different buffers (i.e. if
     * decoding on a GPU). If NULL, will use the allocator from the
     * context
     */
    void (*free_fn) (enum transcoding_pipeline_buffer_id, void*);

    /**
     * Function chosen to read chunk data from the context.
     *
     * Initialized to a default generic read routine, may be updated
     * based on channel information when @sa
     * exr_choose_default_routines is called. This is done such that
     * if the file is uncompressed and the output channel data is
     * planar and the same type, the read function can read straight
     * into the output channels, getting closer to a zero-copy
     * operation. Otherwise a more traditional read, decompress, then
     * unpack pipeline will be used with a default reader.
     *
     * This is allowed to be overridden, but probably is not necessary
     * in most scenarios
     */
    exr_result_t (*read_fn) (struct _exr_decode_pipeline* pipeline);
    /**
     * Function chosen based on the compression type of the part to
     * decompress data.
     *
     * If the user has a custom decompression method for the
     * compression on this part, this can be changed after
     * initialization.
     *
     * If only compressed data is desired, then assign this to NULL
     * after initialization.
     */
    exr_result_t (*decompress_fn) (struct _exr_decode_pipeline* pipeline);
    /**
     * Function chosen based on the output layout of the channels of the part to
     * decompress data.
     *
     * This will be NULL after initialization, until the user
     * specifies a custom routine, or initializes the channel data and
     * calls @sa exr_choose_unpack_routine.
     *
     * If only compressed data is desired, then leave or assign this
     * to NULL after initialization.
     */
    exr_result_t (*unpack_and_convert_fn) (
        struct _exr_decode_pipeline* pipeline);

    /**
     * Small stash of channel info values. This is faster than calling
     * malloc when the channel count in the part is small (RGBAZ),
     * which is super common, however if there are a large number of
     * channels, it will allocate space for that, so do not rely on
     * this being used
     */
    exr_coding_channel_info_t _quick_chan_store[5];
} exr_decode_pipeline_t;

/** initializes the decoding pipeline structure with the channel info for the specified part, and the first block to be read.
 *
 * NB: The unpack_and_convert_fn will be NULL after this. If that
 * stage is desired, initialize the channel output information and
 * call @sa exr_choose_unpack_routine
 */
EXR_EXPORT
exr_result_t exr_initialize_decoding (
    const exr_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_decode_pipeline_t*        decode);

/** Given an initialized decode pipeline, finds appropriate functions
 * to read and shuffle / convert data into the defined channel outputs
 *
 * Calling this is not required if custom routines will be used, or
 * if just the raw compressed data is desired. Although in that scenario, it is probably easier to just read the chunk directly using @sa exr_read_chunk
 */
EXR_EXPORT
exr_result_t exr_decoding_choose_default_routines (
    const exr_context_t    ctxt,
    int                    part_index,
    exr_decode_pipeline_t* decode);

/** Given a decode pipeline previously initialized, updates it for the
 * new chunk to be read.
 *
 * In this manner, memory buffers can be re-used to avoid continual
 * malloc / free calls. Further, it allows the previous choices for
 * the various functions to be quickly re-used.
 */
EXR_EXPORT
exr_result_t exr_decoding_update (
    const exr_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_decode_pipeline_t*        decode);

/** Execute the decoding pipeline */
EXR_EXPORT
exr_result_t exr_decoding_run (
    const exr_context_t ctxt, int part_index, exr_decode_pipeline_t* decode);

/** Free any intermediate memory in the decoding pipeline
 *
 * This does NOT free any pointers referred to in the channel info
 * areas, but rather only the intermediate buffers and memory needed
 * for the structure itself.
 */
EXR_EXPORT
exr_result_t exr_destroy_decoding (
    const exr_context_t ctxt, exr_decode_pipeline_t* decode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_DECODE_H */
