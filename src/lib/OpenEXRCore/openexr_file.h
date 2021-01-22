/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_FILE_H
#define OPENEXR_CORE_FILE_H

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup File Creation Functions
 *
 * @brief These are a group of functions used when creating new files,
 * either for reading, writing, or just updating.
 *
 * @{
 */

/** Starts a default filesystem read. If you have custom I/O
 * requirements, see the corresponding stream function below
 *
 * The error callback is allowed to be null, if so it will default to printing
 * to stderr
 *
 * One notable attribute of the file is that once it has been created
 * via start_read, it has parsed all the header data, and is safe for
 * multiple threads to request data from the same file at the same
 * time.
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_read(
    exr_file_t **file,
    const char *filename,
    exr_error_handler_cb_t error_cb );

/** Starts a default filesystem write. If you have custom I/O requirements,
 * see the corresponding stream function in @see openexr_stream.h
 *
 * The error callback is allowed to be null, if so it will default to printing
 * to stderr
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_write(
    exr_file_t **file,
    const char *filename,
    int use_tempfile,
    exr_error_handler_cb_t error_cb );

/** Starts a default filesystem header update read/write. If you have
 * custom I/O requirements, see the corresponding stream function in
 * @see openexr_stream.h
 *
 * This is a custom mode that allows one to modify the value of a
 * metadata entry, although not to change the size of the header, or
 * any of the image data.
 *
 * The error callback is allowed to be null, if so it will default to
 * printing errors to stderr
 *
 * @returns 0 upon success, otherwise returns a value indicating the error
 */
EXR_EXPORT int exr_start_inplace_header_update(
    exr_file_t **file,
    const char *filename,
    exr_error_handler_cb_t error_cb );

/** @brief Closes and frees any internally allocated memory,
 * calling any provided destroy function for custom streams
 *
 * If the file was opened for write, will first save the chunk offsets
 *
 * @return 0 upon success, error code otherwise. An error will
 * only be able to happen when the file is opened for write.
 */
EXR_EXPORT int exr_close( exr_file_t **file );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_FILE_H */
