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

/* @brief Define a new part in the file.
 *
 * If @param adopt_attr_ownership is non-zero, indicates that the
 * attribute list should be internalized and used as the attributes,
 * avoiding a number of memory copies.
 */
EXR_EXPORT int exr_add_part(
    exr_file_t *file,
    const char *partname,
    exr_STORAGE_TYPE_t type,
    exr_attribute_list_t *attrs,
    int adopt_attr_ownership );

/* @brief This defines a simple one-part file.
 *
 * This is useful to create a simple one-part file with a number of
 * default values as documented below. This is provided as a
 * convenience
 *
 * The data window and display window will be the same, defined by the
 * 4 parameters, @param x_min, @param y_min, @param x_max, @param
 * y_max
 *
 * The data type will be the same for all channels
 *
 * It will be a single-scanline per chunk file, in increasing Y order.
 *
 * The pixel aspect ratio shall be 1.0 (square pixels)
 * The screen window center shall be 0.0, 0.0
 * The screen window width shall be 1.0
 *
 * @param numchans - this currently only allows 3, 4 as input values
 *                   corresponding to BGR or ABGR output respectively
 * @param compression - specify the compression to use
 * 
 * )
 */
EXR_EXPORT int exr_add_simple_part(
    exr_file_t *file,
    const char *partname,
    int numchans,
    exr_PIXEL_TYPE_t pixtype,
    exr_COMPRESSION_TYPE_t compression,
    int x_min,
    int y_min,
    int x_max,
    int y_max );

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
