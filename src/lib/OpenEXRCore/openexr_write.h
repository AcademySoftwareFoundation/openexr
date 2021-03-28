/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_WRITE_H
#define OPENEXR_CORE_WRITE_H

#include "openexr_part.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup File writing functions
 * @brief These are a group of functions used when writing new files
 * @{
 */

/** Structure for negotiating buffers when reading data */
typedef struct
{
    /** 
     * @defgroup Provided Values
     * @brief These values will be filled in by the "tell me the chunk information" function below.
     * @{
     */

    /** channel name, for reference (do not free, refers to internal data structure) */
    const char* channel_name;

    /** number of lines for this channel in this chunk
     *
     * May be 0 or less than overall image height based on sampling
     */
    int height;

    /** width in pixel count
     *
     * May be 0 or less than overall image width based on sampling
     */
    int width;
    /** how many bytes per pixel this channel consumes (i.e. 2 for float16, 4 for float32)*/
    int bytes_per_pel;

    int x_samples;
    int y_samples;
    /** @} */

    /** 
     * @defgroup To be filled in values
     * @brief These values should be filled in prior to calling the "read chunk" function
     * @{
     */

    /** source data for this channel */
    const uint8_t* data_ptr;
    /** increment to get to next pixel in bytes for a scanline */
    int output_pixel_stride;
    /** when lines > 1, increment to get from beginning of line to
     * beginning of next line (in bytes) */
    int output_line_stride;

    /** @} */
} exr_channel_encode_info_t;

typedef struct exr_encode_chunk_info__t
{
    int64_t chunk_data_offset;

    int32_t chunk_idx;
    uint8_t chunk_type;
    uint8_t chunk_compression;

    int16_t channel_count;
    /** Describes the channel information. This information is
     * allocated dynamically, and it and any scratch buffers owned by
     * the decoding process shall be freed by a call to @see
     * destroy_chunk_decoder
     */
    exr_channel_encode_info_t* channels;
    /** faster than calling malloc when the channel count in the part
     * is small (RGBAZ) but do not rely on this being used */
    exr_channel_encode_info_t chan_store[5];

    /** starting y, going either in increasing / decreasing depending on file */
    int start_y;
    /** canonical number of lines in the chunk */
    int height;
    /** canonical width of datawindow in the chunk */
    int width;

    /** When non-zero, indicates the @see destroy_chunk_decoder
     * routine should free any scratch buffers.
     *
     * If you want to take ownership of the scratch buffers for some
     * reason, you can initialize with this on, then change it prior
     * to calling the destroy, and it will leave the buffers in the
     * calling applications care. This can be useful to re-use buffers
     * when reading a range of lines in a single thread.
     */
    int own_scratch_buffers;

    /** scratch buffer used to read the (compressed) data from disk
     *
     * As a custom performance optimization, if the file is
     * uncompressed and the output channel pointers / strides are set
     * to be planar (i.e. output pixel stride is 1), the data will be
     * directly read into the decoded image
     *
     * NB: This buffer will be in file-native layout, where it will
     * either be compressed still, or will be uncompressed with
     * little endian data layout. The application is responsible for
     * interpreting appropriately.
     *
     * Applications must NOT change the size, that is used as the
     * count of bytes to decompress into, instead just provide a
     * buffer that has at least that many bytes.
     */
    struct
    {
        uint64_t size;
        uint8_t* buffer;
    } packed;

    /** scratch buffer used to decompress the data from disk
     *
     * for non-compressed files, this will be unused / 0.
     *
     * This is then used to as a source for re-formatting the channel
     * data into the output format specified in the decode
     * information. If you only wish to decode and perform your own
     * re-formatting of the pixel data, leave the data pointers null
     * in the channel decode info @see channels above.
     *
     * Applications must NOT change the size, that is used as the
     * count of bytes to decompress into, instead just provide a
     * buffer that has at least that many bytes.
     */
    struct
    {
        uint64_t size;
        uint8_t* buffer;
    } unpacked;

    /** for deep files
     *
     * Applications must NOT change the size, that is used as the
     * count of bytes to decompress into, instead just provide a
     * buffer that has at least that many bytes.
     */
    struct
    {
        uint64_t size;
        uint8_t* buffer;
    } sample_table;

    /** spare scratch buffer used to decompress the data from disk
     *
     * for non-compressed files, or compression methods which do
     * not require it, this will be unused / 0.
     *
     * This is used as intermediate storage with some compression
     * formats. If you only wish to extract the compressed / packed
     * data and not decode, leave the data pointer as NULL
     *
     * Applications must NOT change the size, that is used as the
     * count of bytes to decompress into, instead just provide a
     * buffer that has at least that many bytes.
     */
    struct
    {
        uint64_t size;
        uint8_t* buffer;
    } spare;

} exr_encode_chunk_info_t;

EXR_EXPORT exr_result_t exr_encode_chunk_init_scanline (
    const exr_context_t      ctxt,
    int                      part_index,
    exr_encode_chunk_info_t* outinfo,
    int                      y,
    int                      own_scratch_space);

EXR_EXPORT exr_result_t exr_encode_chunk_init_tile (
    const exr_context_t      ctxt,
    int                      part_index,
    exr_encode_chunk_info_t* outinfo,
    int                      tilex,
    int                      tiley,
    int                      levelx,
    int                      levely,
    int                      own_scratch_space);

EXR_EXPORT exr_result_t
exr_write_chunk (exr_context_t ctxt, exr_encode_chunk_info_t* cinfo);

EXR_EXPORT exr_result_t exr_compress_data (
    const exr_context_t     ctxt,
    const exr_compression_t ctype,
    void*                   compressed_data,
    size_t                  comp_buf_size,
    const void*             uncompressed_data,
    size_t                  uncompressed_size);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_WRITE_H */
