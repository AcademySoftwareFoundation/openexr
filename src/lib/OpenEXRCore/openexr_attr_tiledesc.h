/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_TILEDESC_H
#define OPENEXR_ATTR_TILEDESC_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup types and definitions for tile definitions
 * @brief These are a group of types and structures used to represent tiles within OpenEXR.
 * 
 * @{
 */

/** @brief Enum representing what type of tile information is contained */
typedef enum
{
    EXR_DEF(TILE_ONE_LEVEL) = 0, /**< single level of image data */
    EXR_DEF(TILE_MIPMAP_LEVELS) = 1, /**< mipmapped image data */
    EXR_DEF(TILE_RIPMAP_LEVELS) = 2, /**< ripmapped image data */
    EXR_DEF(TILE_LAST_TYPE) /**< guard / out of range type */
} EXR_TYPE(TILE_LEVEL_MODE);

/** @brief Enum representing how to scale positions between levels */
typedef enum
{
    EXR_DEF(TILE_ROUND_DOWN) = 0,
    EXR_DEF(TILE_ROUND_UP) = 1,
    EXR_DEF(TILE_ROUND_LAST_TYPE)
} EXR_TYPE(TILE_ROUND_MODE);

/* tight pack so we can read directly */
#pragma pack(push, 1)

/** @brief Struct holding base tiledesc attribute type defined in spec */
typedef struct
{
    uint32_t x_size;
    uint32_t y_size;
    uint8_t level_and_round;
} EXR_TYPE(attr_tiledesc);

#pragma pack(pop)

/** @brief macro to access type of tiling from packed structure */
#define EXR_GET_TILE_LEVEL_MODE(tiledesc) ((EXR_TYPE(TILE_LEVEL_MODE))(((tiledesc).level_and_round)&0xF))
/** @brief macro to access the rounding mode of tiling from packed structure */
#define EXR_GET_TILE_ROUND_MODE(tiledesc) ((EXR_TYPE(TILE_ROUND_MODE))((((tiledesc).level_and_round)>>4)&0xF))
/** @brief macro to pack the tiling type and rounding mode into packed structure */
#define EXR_PACK_TILE_LEVEL_ROUND(lvl, mode) ((uint8_t)((((uint8_t)((mode) & 0xF)<<4)) | ((uint8_t)((lvl) & 0xF))))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_TILEDESC_H */
