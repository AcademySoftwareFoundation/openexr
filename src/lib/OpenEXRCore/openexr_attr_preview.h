/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_PREVIEW_H
#define OPENEXR_ATTR_PREVIEW_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup preview attribute storage and functions
 * @brief These are a group of functions for handling allocations for a preview attribute
 * @{
 */

/** preview storage struct */
typedef struct
{
    uint32_t width;
    uint32_t height;
    size_t alloc_size; /**< if this is non-zero, the preview owns the data, if 0, is a const ref */
    const uint8_t *rgba;
} exr_attr_preview_t;

/** @brief Allocates memory for a w * h * 4 entry in the preview
 *
 * This presumes the attr_preview passed in is uninitialized prior to this call
 *
 * @param f file for associated preview attribute (used for error reporting)
 * @param p pointer to attribute to fill. Assumed uninitialized
 * @param w width of preview image
 * @param h height of preview image
 *
 * @return 0 on success, error code otherwise
 */
EXR_EXPORT int exr_attr_preview_init(
    exr_file_t *f, exr_attr_preview_t *p, uint32_t w, uint32_t h );

/** @brief Allocates memory for a w * h * 4 entry in the preview and fills with provided data
 *
 * This presumes the attr_preview passed in is uninitialized prior to this call.
 *
 * @param f file for associated preview attribute (used for error reporting)
 * @param p pointer to attribute to fill. Assumed uninitialized
 * @param w width of preview image
 * @param h height of preview image
 * @param d input w * h * 4 bytes of data to copy
 *
 * @return 0 on success, error code otherwise
 */
EXR_EXPORT int exr_attr_preview_create(
    exr_file_t *f, exr_attr_preview_t *p, uint32_t w, uint32_t h, const uint8_t *d );

/** @brief Frees memory for the preview attribute if memory is owned by the preview attr */
EXR_EXPORT void exr_attr_preview_destroy( exr_attr_preview_t *p );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_PREVIEW_H */
