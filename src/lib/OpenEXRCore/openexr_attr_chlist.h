/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_CHLIST_H
#define OPENEXR_ATTR_CHLIST_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"
#include "openexr_attr_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup channel list attribute storage and functions
 * @brief These are a group of functions for handling a list of channels and associated data
 * @{
 */

/** Data types allowed for a channel */
typedef enum
{
    EXR_PIXEL_UINT = 0,
    EXR_PIXEL_HALF = 1,
    EXR_PIXEL_FLOAT = 2,
    EXR_PIXEL_LAST_TYPE
} exr_PIXEL_TYPE_t;

/** Individual channel information*/
typedef struct
{
    exr_attr_string_t name;
    exr_PIXEL_TYPE_t pixel_type; /** Data representation for these pixels: uint, half, float */
    uint8_t p_linear; /**< Possible values are 0 and 1 per docs,
                       * appears deprecated and unused in openexr
                       * lib */
    uint8_t reserved[3];
    int32_t x_sampling;
    int32_t y_sampling;
} exr_attr_chlist_entry_t;

/** List of channel information (sorted alphabetically) */
typedef struct
{
    int num_channels;
    int num_alloced;
    exr_attr_chlist_entry_t *entries;
} exr_attr_chlist_t;

/** @brief initialize a channel list with a number of channels to be added later */
EXR_EXPORT int exr_attr_chlist_init( exr_file_t *f,
                                          exr_attr_chlist_t *chl,
                                          int nchans );

/** @brief Add a channel to the channel list */
EXR_EXPORT int exr_attr_chlist_add( exr_file_t *f,
                                         exr_attr_chlist_t *chl,
                                         const char *name,
                                         exr_PIXEL_TYPE_t ptype,
                                         uint8_t islinear,
                                         int32_t xsamp,
                                         int32_t ysamp );
/** @brief Add a channel to the channel list */
EXR_EXPORT int exr_attr_chlist_add_with_length( exr_file_t *f,
                                                     exr_attr_chlist_t *chl,
                                                     const char *name,
                                                     int32_t namelen,
                                                     exr_PIXEL_TYPE_t ptype,
                                                     uint8_t islinear,
                                                     int32_t xsamp,
                                                     int32_t ysamp );

/** @brief Frees memory for the channel list and all channels inside */
EXR_EXPORT void exr_attr_chlist_destroy( exr_attr_chlist_t * );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_CHLIST_H */
