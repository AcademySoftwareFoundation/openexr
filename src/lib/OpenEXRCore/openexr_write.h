/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_WRITE_H
#define OPENEXR_CORE_WRITE_H

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup File writing functions
 * @brief These are a group of functions used when writing new files
 * @{
 */

EXR_EXPORT int exr_set_longname_support(
    exr_file_t *file, int onoff );

EXR_EXPORT int exr_add_part(
    exr_file_t *file,
    const char *partname,
    exr_STORAGE_TYPE_t type );

EXR_EXPORT int exr_set_part_name(
    exr_file_t *file, int part_index,
    const char *partname );

/** @brief Writes the header data.
 *
 * This has the effect of serving as a transition from "setup" to
 * "writing". It will recompute the number of chunks that will
 * be written, and reset the chunk offsets. If you modify file
 * attributes or part information after a call to this, it will not be
 * reflected in the file unless you call this function again. This
 * then requires you to re-write the image data.
 *
 * @return 0 on success, otherwise an appropriate error code
 */
EXR_EXPORT int exr_write_header(
    exr_file_t *file );

/*
EXR_EXPORT int exr_write_chunk(
    exr_file_t *file,
    const exr_chunk_info_t *cinfo,
    void *compressed_data, size_t comp_buf_size,
    const void *uncompressed_data, size_t uncompressed_size );
*/

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_WRITE_H */
