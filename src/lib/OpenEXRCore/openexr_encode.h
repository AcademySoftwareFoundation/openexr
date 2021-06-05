/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_ENCODE_H
#define OPENEXR_CORE_ENCODE_H

#include "openexr_chunkio.h"
#include "openexr_coding.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure meant to be used on a per-thread basis for writing exr data
 *
 * As should be obvious, this structure is NOT thread safe, but rather
 * meant to be used by separate threads, which can all be accessing
 * the same context concurrently.
 */
typedef struct _exr_encode_pipeline
{
    /** the output channel information for this chunk
     *
     * User is expected to fill the channel pointers for the input
     * channels. For writing, all channels must be initialized prior
     * to using @sa exr_choose_pack_routine. If a custom pack routine
     * is written, that is up to the implementor.
     *
     * Describes the channel information. This information is
     * allocated dynamically during @sa exr_initialize_encoding
     */
    exr_coding_channel_info_t* channels;
    int16_t                    channel_count;

    uint8_t pad[2];

    /** copy of the parameters given to the initialize / update for convenience */
    int                    part_index;
    exr_const_context_t    context;
    exr_chunk_block_info_t chunk_block;

    /**
     * can be used by the user to pass custom context data through
     * the encode pipeline
     */
    void* encoding_user_data;

    /** the (compressed) buffer.
     *
     * If null, will be allocated during the run of the pipeline.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* packed_buffer;
    /** for deep data */
    uint64_t packed_bytes;

    /** used when re-using the same encode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     *
     * If null, will be allocated during the run of the pipeline.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    size_t packed_alloc_size;
    /** for deep data */
    void* packed_sample_count_table;
    /** for deep data */
    size_t packed_sample_count_bytes;
    /** for deep data */
    size_t packed_sample_count_alloc_size;

    /** the compressed buffer, only needed for
     * compressed files
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* compressed_buffer;
    /** must be filled in as the pipeline runs to inform the writing
     * software about the compressed size of the chunk (if it is an
     * uncompressed file or the compression would make the file
     * larger, it is expected to be the packed_buffer)
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to zero here. Be cognizant of any
     * custom allocators.
     */
    size_t compressed_bytes;
    /** used when re-using the same encode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to zero here. Be cognizant of any
     * custom allocators.
     */
    size_t compressed_alloc_size;

    /** a scratch buffer for intermediate results
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* scratch_buffer_1;
    /** used when re-using the same encode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    size_t scratch_alloc_size_1;
    /** some compression routines may need a second scratch buffer
     *
     * If null, will be allocated during the run of the pipeline when
     * needed.
     *
     * If the caller wishes to take control of the buffer, simple
     * adopt the pointer and set it to null here. Be cognizant of any
     * custom allocators.
     */
    void* scratch_buffer_2;
    /** used when re-using the same encode pipeline struct to know if
     * chunk is changed size whether current buffer is large enough */
    size_t scratch_alloc_size_2;

    /**
     * enables a custom allocator for the different buffers (i.e. if
     * encoding on a GPU). If NULL, will use the allocator from the
     * context
     */
    void* (*alloc_fn) (enum transcoding_pipeline_buffer_id, size_t);
    /**
     * enables a custom allocator for the different buffers (i.e. if
     * encoding on a GPU). If NULL, will use the allocator from the
     * context
     */
    void (*free_fn) (enum transcoding_pipeline_buffer_id, void*);

    /**
     * Function chosen based on the output layout of the channels of the part to
     * decompress data.
     *
     * If the user has a custom method for the
     * compression on this part, this can be changed after
     * initialization.
     */
    exr_result_t (*convert_and_pack_fn) (struct _exr_encode_pipeline* pipeline);

    /**
     * Function chosen based on the compression type of the part to
     * compress data.
     *
     * If the user has a custom compression method for the compression
     * type on this part, this can be changed after initialization.
     */
    exr_result_t (*compress_fn) (struct _exr_encode_pipeline* pipeline);

    /**
     * This routine is used when waiting for other threads to finish
     * writing previous chunks such that this thread can write this
     * chunk. This is used for parts which have a specified chunk
     * ordering (increasing / decreasing y) and the chunks can not be
     * written randomly (as could be true for uncompressed).
     *
     * This enables the calling application to contribute thread time
     * to other computation as needed, or just use something like
     * pthread_yield().
     *
     * By default, this routine will be assigned to a function which
     * returns an error, failing the encode immediately. In this way,
     * it assumes that there is only one thread being used for
     * writing.
     *
     * It is up to the user to provide an appropriate routine if
     * performing multi-threaded writing.
     */
    exr_result_t (*yield_until_ready_fn) (
        struct _exr_encode_pipeline* pipeline);

    /**
     * Function chosen to write chunk data to the context.
     *
     * This is allowed to be overridden, but probably is not necessary
     * in most scenarios
     */
    exr_result_t (*write_fn) (struct _exr_encode_pipeline* pipeline);

    /**
     * Small stash of channel info values. This is faster than calling
     * malloc when the channel count in the part is small (RGBAZ),
     * which is super common, however if there are a large number of
     * channels, it will allocate space for that, so do not rely on
     * this being used
     */
    exr_coding_channel_info_t _quick_chan_store[5];
} exr_encode_pipeline_t;

/** initializes the encoding pipeline structure with the channel info
 * for the specified part, and the first block to be written.
 *
 * NB: The pack_and_convert_fn will be NULL after this. If that
 * stage is desired, initialize the channel output information and
 * call @sa exr_choose_unpack_routine
 */
EXR_EXPORT
exr_result_t exr_encoding_initialize (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_encode_pipeline_t*        encode_pipe);

/** Given an initialized encode pipeline, finds an appropriate
 * function to shuffle and convert data into the defined channel
 * outputs
 *
 * Calling this is not required if a custom routine will be used, or
 * if just the raw decompressed data is desired.
 */
EXR_EXPORT
exr_result_t exr_encoding_choose_default_routines (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode_pipe);

/** Given a encode pipeline previously initialized, updates it for the
 * new chunk to be written.
 *
 * In this manner, memory buffers can be re-used to avoid continual
 * malloc / free calls. Further, it allows the previous choices for
 * the various functions to be quickly re-used.
 */
EXR_EXPORT
exr_result_t exr_encoding_update (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_encode_pipeline_t*        encode_pipe);

/** Execute the encoding pipeline */
EXR_EXPORT
exr_result_t exr_encoding_run (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode_pipe);

/** Free any intermediate memory in the encoding pipeline
 *
 * This does NOT free any pointers referred to in the channel info
 * areas, but rather only the intermediate buffers and memory needed
 * for the structure itself.
 */
EXR_EXPORT
exr_result_t exr_encoding_destroy (
    exr_const_context_t ctxt, exr_encode_pipeline_t* encode_pipe);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_ENCODE_H */
