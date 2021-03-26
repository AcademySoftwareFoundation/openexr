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
EXR_EXPORT const exr_attr_chlist_t *exr_get_channels(
    exr_file_t *file, int part_index );

/** @brief Defines a new channel to the output file part. */
EXR_EXPORT int exr_add_channels(
    exr_file_t *file, int part_index,
    const char *name,
    exr_PIXEL_TYPE_t ptype,
    uint8_t islinear,
    int32_t xsamp, int32_t ysamp );

/** @brief Copies the files from a source.
 *
 * Useful if you are manually constructing the list or simply copying
 * from an input file */
EXR_EXPORT int exr_set_channels(
    exr_file_t *file, int part_index,
    const exr_attr_chlist_t *channels );

/** @brief Retrieves the compression method used for the specified part (use 0 for single part images). */
EXR_EXPORT exr_compression_t exr_get_compression(
    exr_file_t *file, int part_index );
/** @brief Sets the compression method used for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_compression(
    exr_file_t *file, int part_index, exr_compression_t ctype );

/** @brief Retrieves the data window for the specified part (use 0 for single part images). */
EXR_EXPORT exr_attr_box2i_t exr_get_data_window(
    exr_file_t *file, int part_index );
/** @brief Sets the data window for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_data_window(
    exr_file_t *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max );

/** @brief Retrieves the display window for the specified part (use 0 for single part images). */
EXR_EXPORT exr_attr_box2i_t exr_get_display_window(
    exr_file_t *file, int part_index );
/** @brief Sets the display window for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_display_window(
    exr_file_t *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max );

/** @brief Retrieves the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT exr_lineorder_t exr_get_line_order(
    exr_file_t *file, int part_index );
/** @brief Sets the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_line_order(
    exr_file_t *file, int part_index, exr_lineorder_t lo );

/** @brief Retrieves the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT float exr_get_pixel_aspect_ratio(
    exr_file_t *file, int part_index );
/** @brief Sets the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_pixel_aspect_ratio(
    exr_file_t *file, int part_index, float par );

/** @brief Retrieves the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT exr_attr_v2f_t exr_get_screen_window_center(
    exr_file_t *file, int part_index );
/** @brief Sets the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_screen_window_center(
    exr_file_t *file, int part_index,
    float x, float y );

/** @brief Retrieves the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT float exr_get_screen_window_width(
    exr_file_t *file, int part_index );
/** @brief Sets the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_set_screen_window_width(
    exr_file_t *file, int part_index, float ssw );

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
EXR_EXPORT uint32_t exr_get_tile_x_size(
    exr_file_t *file, int part_index );
/** @brief Retrieves the y (vertical) direction tile size for a tiled part (use 0 for single part images). */
EXR_EXPORT uint32_t exr_get_tile_y_size(
    exr_file_t *file, int part_index );
/** @brief Retrieves the tiling mode for a tiled part (use 0 for single part images). */
EXR_EXPORT exr_TILE_LEVEL_MODE_t exr_get_tile_level_mode(
    exr_file_t *file, int part_index );
/** @brief Retrieves the tile rounding mode for a tiled part (use 0 for single part images). */
EXR_EXPORT exr_TILE_ROUND_MODE_t exr_get_tile_round_mode(
    exr_file_t *file, int part_index );
/** @brief Sets the tiling info for a tiled part (use 0 for single part images). */
EXR_EXPORT int exr_set_tile_descriptor(
    exr_file_t *file, int part_index,
    uint32_t x_size, uint32_t y_size,
    exr_TILE_LEVEL_MODE_t level_mode,
    exr_TILE_ROUND_MODE_t round_mode );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_REQ_ATTR_H */
