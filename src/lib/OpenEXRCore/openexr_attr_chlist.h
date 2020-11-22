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
    EXR_DEF(PIXEL_UINT) = 0,
    EXR_DEF(PIXEL_HALF) = 1,
    EXR_DEF(PIXEL_FLOAT) = 2,
    EXR_DEF(PIXEL_LAST_TYPE)
} EXR_TYPE(PIXEL_TYPE);

/** Individual channel information*/
typedef struct
{
    EXR_TYPE(attr_string) name;
    EXR_TYPE(PIXEL_TYPE) pixel_type; /** Data representation for these pixels: uint, half, float */
    uint8_t p_linear; /**< Possible values are 0 and 1 per docs,
                       * appears deprecated and unused in openexr
                       * lib */
    uint8_t reserved[3];
    int32_t x_sampling;
    int32_t y_sampling;
} EXR_TYPE(attr_chlist_entry);

/** List of channel information (sorted alphabetically) */
typedef struct
{
    int num_channels;
    int num_alloced;
    EXR_TYPE(attr_chlist_entry) *entries;
} EXR_TYPE(attr_chlist);

/** @brief initialize a channel list with a number of channels to be added later */
EXR_EXPORT int EXR_FUN(attr_chlist_init)( EXR_TYPE(FILE) *f,
                                          EXR_TYPE(attr_chlist) *chl,
                                          int nchans );

/** @brief Add a channel to the channel list */
EXR_EXPORT int EXR_FUN(attr_chlist_add)( EXR_TYPE(FILE) *f,
                                         EXR_TYPE(attr_chlist) *chl,
                                         const char *name,
                                         EXR_TYPE(PIXEL_TYPE) ptype,
                                         uint8_t islinear,
                                         int32_t xsamp,
                                         int32_t ysamp );
/** @brief Add a channel to the channel list */
EXR_EXPORT int EXR_FUN(attr_chlist_add_with_length)( EXR_TYPE(FILE) *f,
                                                     EXR_TYPE(attr_chlist) *chl,
                                                     const char *name,
                                                     int32_t namelen,
                                                     EXR_TYPE(PIXEL_TYPE) ptype,
                                                     uint8_t islinear,
                                                     int32_t xsamp,
                                                     int32_t ysamp );

/** @brief Frees memory for the channel list and all channels inside */
EXR_EXPORT void EXR_FUN(attr_chlist_destroy)( EXR_TYPE(attr_chlist) * );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_CHLIST_H */
