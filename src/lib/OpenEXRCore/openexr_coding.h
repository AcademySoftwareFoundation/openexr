/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_CODING_H
#define OPENEXR_CORE_CODING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enum for use in a custom allocator in the encode / decode pipelines
 * (i.e. so the implementor knows whether to allocate on which device
 * based on the buffer disposition)
 */
enum transcoding_pipeline_buffer_id
{
    EXR_TRANSCODE_BUFFER_PACKED,
    EXR_TRANSCODE_BUFFER_UNPACKED,
    EXR_TRANSCODE_BUFFER_SCRATCH1,
    EXR_TRANSCODE_BUFFER_SCRATCH2,
    EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
    EXR_TRANSCODE_BUFFER_SAMPLES
};

/**
 * Enum indicating the data type in the buffer
 */
enum transcoding_pipeline_buffer_type
{
    EXR_BUFFER_TYPE_HALF,
    EXR_BUFFER_TYPE_FLOAT,
    EXR_BUFFER_TYPE_UINT32
};

/** @brief Structure for negotiating buffers when decoding / encoding
 * chunks of data
 *
 * This is generic and meant to negotiate exr data bi-directionally,
 * in that the same structure is used for both decoding and encoding
 * chunks for read and write, respectively.
 *
 * The first 4 elements are meant to be controlled by the caller of
 * the encode / decode routines and provide memory to the library to
 * either read from or fill up.
 */
typedef struct
{
    /**
     * This data member has different requirements reading vs
     * writing. When reading, if this is left as NULL, the channel
     * will be skipped during read and not filled in.  During a write
     * operation, this pointer is considered const and not
     * modified. To make this more clear, a union is used here.
     */
    union
    {
        uint8_t*       decode_to_ptr;
        const uint8_t* encode_from_ptr;
    };
    /** increment to get to next pixel
     *
     * This is in bytes. Must be specified if the pointer above is
     * specified.
     *
     * This is useful for implementing transcoding generically of planar or interleaved data. For planar data, where the layout is RRRRRGGGGGBBBBB, you can pass in 1 * bytes per component
     */
    int output_pixel_stride;
    /**
     * When lines > 1 for a chunk, this is the increment used to get
     * from beginning of line to beginning of next line.
     *
     * This is in bytes. Must be specified if the pointer above is
     * specified.
     */
    int output_line_stride;

    /**
     * how many bytes per pixel the output should be (i.e. 2 for
     * float16, 4 for float32 / uint32)
     */
    int output_bytes_per_element;

    /***** Elements below are populated by the library *****/

    /** number of lines for this channel in this chunk
     *
     * May be 0 or less than overall image height based on sampling
     * (i.e. when in 4:2:0 type sampling)
     */
    int height;

    /** width in pixel count
     *
     * May be 0 or less than overall image width based on sampling
     * (i.e. 4:2:2 will have some channels have fewer values)
     */
    int width;

    /**
     * how many bytes per pixel this channel consumes (i.e. 2 for
     * float16, 4 for float32 / uint32)
     */
    int bytes_per_element;

    /** horizontal subsampling information */
    int32_t x_samples;
    /** vertical subsampling information */
    int32_t y_samples;

    /** channel name
     *
     * This is provided as a convenient reference. Do not free, this
     * refers to the internal data structure in the context.
     */
    const char* channel_name;
} exr_coding_channel_info_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_CODING_H */
