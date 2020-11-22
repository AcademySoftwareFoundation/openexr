/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_REQ_ATTR_H
#define OPENEXR_CORE_REQ_ATTR_H

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup required attribute utililities
 *
 * @brief These are a group of functions for attributes that are
 * required to be in every part of every file.
 *
 * @{
 */

/** @brief retrieves the list of channels */
EXR_EXPORT const EXR_TYPE(attr_chlist) *EXR_FUN(get_channels)(
    EXR_TYPE(FILE) *file, int part_index );

/** @brief Defines a new channel to the output file part. */
EXR_EXPORT int EXR_FUN(add_channels)(
    EXR_TYPE(FILE) *file, int part_index,
    const char *name,
    EXR_TYPE(PIXEL_TYPE) ptype,
    uint8_t islinear,
    int32_t xsamp, int32_t ysamp );
/** @brief Copies the files from a source.
 *
 * Useful if you are manually constructing the list or simply copying
 * from an input file */
EXR_EXPORT int EXR_FUN(set_channels)(
    EXR_TYPE(FILE) *file, int part_index,
    const EXR_TYPE(attr_chlist) *channels );

/** @brief Retrieves the compression method used for the specified part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(COMPRESSION_TYPE) EXR_FUN(get_compression)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the compression method used for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_compression)(
    EXR_TYPE(FILE) *file, int part_index, EXR_TYPE(COMPRESSION_TYPE) ctype );

/** @brief Retrieves the data window for the specified part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(attr_box2i) EXR_FUN(get_data_window)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the data window for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_data_window)(
    EXR_TYPE(FILE) *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max );

/** @brief Retrieves the display window for the specified part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(attr_box2i) EXR_FUN(get_display_window)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the display window for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_display_window)(
    EXR_TYPE(FILE) *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max );

/** @brief Retrieves the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(LINEORDER_TYPE) EXR_FUN(get_line_order)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_line_order)(
    EXR_TYPE(FILE) *file, int part_index, EXR_TYPE(LINEORDER_TYPE) lo );

/** @brief Retrieves the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT float EXR_FUN(get_pixel_aspect_ratio)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_pixel_aspect_ratio)(
    EXR_TYPE(FILE) *file, int part_index, float par );

/** @brief Retrieves the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(attr_v2f) EXR_FUN(get_screen_window_center)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_screen_window_center)(
    EXR_TYPE(FILE) *file, int part_index,
    float x, float y );

/** @brief Retrieves the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT float EXR_FUN(get_screen_window_width)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_screen_window_width)(
    EXR_TYPE(FILE) *file, int part_index, float ssw );

/** @} */

/** 
 * @defgroup required attribute utililities depending on file type
 *
 * @brief These are a group of functions for attributes that are
 * required to be in every part of every file, when they are that type.
 *
 * @{
 */

/** @brief Retrieves the x (horizontal) direction tile size for a tiled part (use 0 for single part images). */
EXR_EXPORT uint32_t EXR_FUN(get_tile_x_size)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Retrieves the y (vertical) direction tile size for a tiled part (use 0 for single part images). */
EXR_EXPORT uint32_t EXR_FUN(get_tile_y_size)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Retrieves the tiling mode for a tiled part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(TILE_LEVEL_MODE) EXR_FUN(get_tile_level_mode)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Retrieves the tile rounding mode for a tiled part (use 0 for single part images). */
EXR_EXPORT EXR_TYPE(TILE_ROUND_MODE) EXR_FUN(get_tile_round_mode)(
    EXR_TYPE(FILE) *file, int part_index );
/** @brief Sets the tiling info for a tiled part (use 0 for single part images). */
EXR_EXPORT int EXR_FUN(set_tile_descriptor)(
    EXR_TYPE(FILE) *file, int part_index,
    uint32_t x_size, uint32_t y_size,
    EXR_TYPE(TILE_LEVEL_MODE) level_mode,
    EXR_TYPE(TILE_ROUND_MODE) round_mode );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_REQ_ATTR_H */
