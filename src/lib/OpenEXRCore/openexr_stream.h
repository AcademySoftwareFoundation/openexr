/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_STREAM_H
#define OPENEXR_CORE_STREAM_H

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup Stream / File Functions
 *
 * @brief These are a group of function interfaces used when
 * interacting with files. This allows one to customize the I/O
 * behavior of OpenEXR, or abstract other file system types.
 *
 * @{
 */

#if defined(_WIN32) || defined(_WIN64)
/* msvc 2015+ has off_t in stdint.h / sys/types.h, we can use that as it's signed */
typedef off_t exr_ssize_t;
#else
typedef ssize_t exr_ssize_t;
#endif

/** Destroy custom stream function pointer
 *
 *  Generic callback to clean up user data for custom streams.
 *  This is called when the file is closed and expected not to
 *  error
 *
 *  @param failed - indicates the write operation failed, the
 *                  implementor may wish to cleanup temporary
 *                  files
 */
typedef void (* exr_destroy_stream_func_ptr_t )( exr_file_t *file, void *userdata, int failed );

/** Query stream size function pointer
 *
 * Used to query the size of the file, or amount of data representing
 * the openexr file in the data stream.
 *
 * This is used to validate requests against the file. If the size is
 * unavailable, return -1, which will disable these validation steps
 * for this file, although appropriate memory safeguards must be in
 * place in the calling application.
 */
typedef exr_ssize_t (* exr_query_size_func_ptr_t )( exr_file_t *file, void *userdata );

/** Read custom stream function pointer
 *
 *  Used to read data from a custom stream. Expects similar semantics to
 *  pread or ReadFile with overlapped data under win32
 *
 *  It is required that this provides thread-safe concurrent access to
 *  the same file. If the stream layer you are providing does not have
 *  this guarantee, your are responsible for providing appropriate
 *  serialization of requests.
 *
 *  A file should be expected to be accessed in the following pattern:
 *  - upon open, the header and part information attributes will be read
 *  - upon the first image read request, the offset tables will be read
 *    multiple threads accessing this concurrently may actually read
 *    these values at the same time
 *  - chunks can then be read in any order as preferred by the
 *    application
 */
typedef exr_ssize_t (* exr_read_func_ptr_t )(
    exr_file_t *file,
    void *userdata, void *buffer, size_t sz, off_t offset,
    exr_stream_error_func_ptr_t error_cb );

/** Write custom stream function pointer
 *
 *  Used to write data to a custom stream. Expects similar semantics to
 *  pwrite or WriteFile with overlapped data under win32
 *
 *  It is required that this provides thread-safe concurrent access to
 *  the same file. While it is unlikely that multiple threads will
 *  be used to write data for compressed forms, it is possible.
 *
 *  A file should be expected to be accessed in the following pattern:
 *  - upon open, the header and part information attributes is constructed
 *  - when the write_header routine is called, the header becomes immutable
 *    and is written to the file. This computes the space to store the chunk
 *    offsets, but does not yet write the values
 *  - Image chunks are written to the file, and appear in the order they
 *    are written, not in the ordering that is required by the chunk offset
 *    table (unless written in that order)
 *  - at file close, the chunk offset tables are written to the file
 */
typedef exr_ssize_t (* exr_write_func_ptr_t )(
    exr_file_t *file,
    void *userdata, const void *buffer, size_t sz, off_t offset,
    exr_stream_error_func_ptr_t error_cb );

/** @} */

/** 
 * @defgroup Stream Creation Functions
 *
 * @brief These are a group of functions used when creating new files,
 * using custom I/O routines.
 *
 * Streams should be closed using the same close routine as normal
 * files.
 *
 * @{
 */

/** Starts a custom stream read.
 *
 * Use the same close routine as a normal file to shutdown the file,
 * when this is called, the destroy function will be called.
 * 
 * The error callback is allowed to be null, if so it will default to printing
 * errors to stderr
 *
 * One notable attribute of the file is that once this routine has
 * created a file and returned, it has parsed all the header data, and
 * is safe for multiple threads to request data concurrently.
 *
 * The provided @see read_fn must expect this if the application is
 * multi-threaded in this manner.
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_read_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_read_func_ptr_t read_fn,
    exr_query_size_func_ptr_t size_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb );

/** Starts a custom stream write.
 *
 * Use the same close routine as a normal file to shutdown the file,
 * when this is called, the destroy function will be called.
 * 
 * The error callback is allowed to be null, if so it will default to printing
 * errors to stderr
 *
 * Most files are expected to be written in a particular layout
 * (increasing or decreasing y), and so threading is not expected to
 * be used during write. However, it is allowed for some cases (random
 * y or multiple parts), and so must be expected if the application is
 * multi-threaded in this manner.
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_write_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_write_func_ptr_t write_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb );

/** Starts a header update read/write file.
 *
 * This is a custom mode that allows one to modify the value of a
 * metadata entry, although not to change the size of the header, or
 * any of the image data.
 *
 * When the file is closed, the destroy function will be called.
 *
 * The error callback is allowed to be null, if so it will default to printing
 * errors to stderr
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_inplace_header_update_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_read_func_ptr_t read_fn,
    exr_query_size_func_ptr_t size_fn,
    exr_write_func_ptr_t write_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_STREAM_H */
