/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_SIMPLE_H
#define OPENEXR_ATTR_SIMPLE_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup simple attribute value values
 * @brief These are a group of enum values defining valid values for some attributes
 *
 * @{
 */

/** enum declaring allowed values for uint8_t value stored in built-in compression type */
typedef enum
{
    EXR_COMPRESSION_NONE = 0,
    EXR_COMPRESSION_RLE = 1,
    EXR_COMPRESSION_ZIPS = 2,
    EXR_COMPRESSION_ZIP = 3,
    EXR_COMPRESSION_PIZ = 4,
    EXR_COMPRESSION_PXR24 = 5,
    EXR_COMPRESSION_B44 = 6,
    EXR_COMPRESSION_B44A = 7,
    EXR_COMPRESSION_DWAA = 8,
    EXR_COMPRESSION_DWAB = 9,
    EXR_COMPRESSION_LAST_TYPE /**< invalid value, provided for range checking */
} exr_COMPRESSION_TYPE_t;

/** enum declaring allowed values for uint8_t value stored in built-in env map type */
typedef enum
{
    EXR_ENVMAP_LATLONG = 0,
    EXR_ENVMAP_CUBE = 1,
    EXR_ENVMAP_LAST_TYPE /**< invalid value, provided for range checking */
} exr_ENVMAP_TYPE_t;

/** enum declaring allowed values for uint8_t value stored in lineOrder type */
typedef enum
{
    EXR_LINEORDER_INCREASING_Y = 0,
    EXR_LINEORDER_DECREASING_Y = 1,
    EXR_LINEORDER_RANDOM_Y = 2,
    EXR_LINEORDER_LAST_TYPE /**< invalid value, provided for range checking */
} exr_LINEORDER_TYPE_t;

/** enum declaring allowed values for part type */
typedef enum
{
    EXR_STORAGE_SCANLINE = 0, /**< corresponds to type of 'scanlineimage' */
    EXR_STORAGE_TILED, /**< corresponds to type of 'tiledimage' */
    EXR_STORAGE_DEEP_SCANLINE, /**< corresponds to type of 'deepscanline' */
    EXR_STORAGE_DEEP_TILED, /**< corresponds to type of 'deeptile' */
    EXR_STORAGE_LAST_TYPE /**< invalid value, provided for range checking */
} exr_STORAGE_TYPE_t;

/** @} */

/** 
 * @defgroup simple attribute value structs
 * @brief These are a group of structures used to hold the values for predefined types
 *
 * The layout of these should be compatible with the corresponding
 * C++ type in Imath, however to enable C-only usage, we define the
 * simplistic structure here.
 * 
 * @{
 */

/* Most are naturally aligned, but force some of these
 * structs to be tightly packed */
#pragma pack(push, 1)

/** @brief struct to hold an integer box / region definition */
typedef struct
{
    int32_t x_min;
    int32_t y_min;
    int32_t x_max;
    int32_t y_max;
} exr_attr_box2i_t;

/** @brief struct to hold a floating-point box / region definition */
typedef struct
{
    float x_min;
    float y_min;
    float x_max;
    float y_max;
} exr_attr_box2f_t;

/** @brief struct to hold color chromaticities to interpret the tristimulus color values in the image data */
typedef struct
{
    float red_x;
    float red_y;
    float green_x;
    float green_y;
    float blue_x;
    float blue_y;
    float white_x;
    float white_y;
} exr_attr_chromaticities_t;

/** @brief struct to hold keycode information */
typedef struct
{
    int32_t film_mfc_code;
    int32_t film_type;
    int32_t prefix;
    int32_t count;
    int32_t perf_offset;
    int32_t perfs_per_frame;
    int32_t perfs_per_count;
} exr_attr_keycode_t;

/** @brief struct to hold a 32-bit floating-point 3x3 matrix */
typedef struct
{
    float m[9];
} exr_attr_m33f_t;

/** @brief struct to hold a 64-bit floating-point 3x3 matrix */
typedef struct
{
    double m[9];
} exr_attr_m33d_t;

/** @brief struct to hold a 32-bit floating-point 4x4 matrix */
typedef struct
{
    float m[16];
} exr_attr_m44f_t;

/** @brief struct to hold a 64-bit floating-point 4x4 matrix */
typedef struct
{
    double m[16];
} exr_attr_m44d_t;

/** @brief struct to hold an integer ratio value */
typedef struct
{
    int32_t num;
    uint32_t denom;
} exr_attr_rational_t;

/** @brief struct to hold timecode information */
typedef struct
{
    uint32_t time_and_flags;
    uint32_t user_data;
} exr_attr_timecode_t;

/** @brief struct to hold a 2-element integer vector */
typedef struct
{
    union 
    {
        struct 
        {
            int32_t x, y;
        };
        int32_t arr[2];
    };
} exr_attr_v2i_t;

/** @brief struct to hold a 2-element 32-bit float vector */
typedef struct
{
    union 
    {
        struct 
        {
            float x, y;
        };
        float arr[2];
    };
} exr_attr_v2f_t;

/** @brief struct to hold a 2-element 64-bit float vector */
typedef struct
{
    union 
    {
        struct 
        {
            double x, y;
        };
        double arr[2];
    };
} exr_attr_v2d_t;

/** @brief struct to hold a 3-element integer vector */
typedef struct
{
    union 
    {
        struct 
        {
            int32_t x, y, z;
        };
        int32_t arr[3];
    };
} exr_attr_v3i_t;

/** @brief struct to hold a 3-element 32-bit float vector */
typedef struct
{
    union 
    {
        struct 
        {
            float x, y, z;
        };
        float arr[3];
    };
} exr_attr_v3f_t;

/** @brief struct to hold a 3-element 64-bit float vector */
typedef struct
{
    union 
    {
        struct 
        {
            double x, y, z;
        };
        double arr[3];
    };
} exr_attr_v3d_t;

#pragma pack(pop)
/* Done with structures we'll read directly... */

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_SIMPLE_H */
