/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_READ_H
#define OPENEXR_CORE_READ_H

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup Reading
 * @brief These are a group of functions used when reading data from files
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
    const char *channel_name;

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

    /** if left to NULL, will skip writing this channel */
    uint8_t *data_ptr;
    /** increment to get to next pixel in bytes for a scanline */
    int output_pixel_stride;
    /** when lines > 1, increment to get from beginning of line to
     * beginning of next line (in bytes) */
    int output_line_stride; 

    /** @} */
} exr_channel_decode_info_t;

typedef struct exr_decode_chunk_info__t
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
    exr_channel_decode_info_t *channels;
    /** faster than calling malloc when the channel count in the part
     * is small (RGBAZ) but do not rely on this being used */
    exr_channel_decode_info_t chan_store[5];

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
        uint8_t *buffer;
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
        uint8_t *buffer;
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
        uint8_t *buffer;
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
        uint8_t *buffer;
    } spare;

} exr_decode_chunk_info_t;

/** returns the maximum unpacked size of a chunk for the file part.
 *
 * This may be used ahead of any actual reading of data, so can be
 * used to pre-allocate buffers for multiple threads in one block or
 * whatever your application may require.
 */
EXR_EXPORT uint64_t exr_get_chunk_unpacked_size( exr_file_t *file, int part_index );

/** @brief Computes the chunk_info for a particular scanline.
 *
 * Note that this may read data from the file to query the compressed
 * size of the data when the file is compressed or a deep scanline
 * file, so is not always a cheap operation, so should only be called
 * once per chunk request if possible (although this read must happen
 * at some point).
 *
 * Of further note is that the resulting number of scanlines and size
 * depends on the compression style used, and so may return a range of
 * scanlines that will be returned
 *
 * @param file File to be decoded
 * @param part_index Part index within the file
 * @param outinfo Structure to be initialized and filled in
 * @param y Scanline requested
 * @param own_scratch_space Sets the flag to indicate that the library
 *                          should allocate any necessary internal
 *                          scratch space. When 0, the application must
 *                          provide memory, or only request packed
 *                          buffers
 *
 * @return 0 upon success, otherwise an appropriate error code
 */
EXR_EXPORT int exr_decode_chunk_init_scanline(
    exr_file_t *file, int part_index,
    exr_decode_chunk_info_t *outinfo,
    int y, int own_scratch_space );

/** @brief Initializes the chunk_info for a particular tile.
 *
 * Note that this may read data from the file to query the compressed
 * size of the data when the file is compressed or a deep scanline
 * file, so is not always a cheap operation, so should only be called
 * once per chunk / tile if possible.
 *
 * @param file File to be decoded
 * @param part_index Part index within the file
 * @param outinfo Structure to be initialized and filled in
 * @param tilex x position in tile space (i.e. tile counts, not pixel)
 * @param tilex y position in tile space (i.e. tile counts, not pixel)
 * @param levelx x mip / rip level to read
 *               (must be 0 for single-level tile images)
 * @param levely y mip / rip level to read
 *               (must be 0 for single-level tile images, must be same as
 *                levelx for mip level files)
 * @param own_scratch_space Sets the flag to indicate that the library
 *                          should allocate any necessary internal
 *                          scratch space. When 0, the application must
 *                          provide memory, or only request packed
 *                          buffers
 *
 * @return 0 upon success, otherwise an appropriate error code
 */
EXR_EXPORT int exr_decode_chunk_init_tile(
    exr_file_t *file, int part_index,
    exr_decode_chunk_info_t *outinfo,
    int tilex, int tiley,
    int levelx, int levely,
    int own_scratch_space );

/** @brief Free any allocated data owned by the decode_chunk_info struct */
EXR_EXPORT void exr_destroy_decode_chunk_info(
    exr_decode_chunk_info_t *outinfo );

/** @brief Read a chunk from a file.
 *
 * This is only valid on a file that has been opened for read. This is
 * the only access mechanism for reading data from the file. Do note
 * that this is only provided on a per-chunk basis. The C++ layer has
 * buffering and other conveniences to read the entire image in a
 * single call, or just a single scanline at a time.
 *
 * Steps performed:
 * 1. Read the (compressed) data into the compressed scratch space,
 *    allocating first if own_scratch_space is non-zero.
 * 2. Decompress as appropriate. This may allocate a scratch buffer
 *    for unpacked if needed and own_scratch_space is non-zero.
 *    NB: If own_scratch_space is 0 and the unpack buffer is NULL,
 *        the routine will return at this point, allowing the user
 *        to provide custom decompression calls
 * 3. If pointers and stride information are provided to the channel
 *    decode information list, propagate the decompressed data into
 *    the output.
 *
 * if own_scratch_space is zero, buffers must be provided. In this
 * mode, reading will stop as soon as a buffer needed is NULL, but
 * after the read of the compressed data, will not report an error.
 * In this way, one is able to read the compressed data only, or only
 * read and decompress, or go all the way to decoding into output
 * pixel locations. Do note that some interpretation of the
 * compression type is performed, in that if the data is not
 * compressed, the intermediate buffers are not needed, and so the
 * routine can jump straight to reading data into the output as a
 * "minimal-copy" (ignoring O.S. level buffering, zero-copy).
 *
 * @param file The file to read from
 * @param cinfo The chunk to read, as computed using
 *              @sa compute_chunk_for_scanline or @sa compute_chunk_for_tile.
 */
EXR_EXPORT int exr_read_chunk(
    exr_file_t *file,
    exr_decode_chunk_info_t *cinfo );

/** @brief Perform buffer decompression
 * TODO: This will need more info for all the compression types
 *       like b44 and pxr24 that do different things depending on
 *       the underlying buffer type
 */
EXR_EXPORT int exr_decompress_data(
    exr_file_t *f,
    const exr_COMPRESSION_TYPE_t ctype,
    void *compressed_data, size_t comp_buf_size,
    void *uncompressed_data, size_t uncompressed_size );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_READ_H */
