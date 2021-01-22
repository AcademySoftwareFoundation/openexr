/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_STRING_H
#define OPENEXR_ATTR_STRING_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup string attribute storage and functions
 * @brief These are a group of functions for handling a string in the attributes
 * @{
 */

/** Storage for a string */
typedef struct
{
    int32_t length;
    int32_t alloc_size; /**< if this is non-zero, the string owns the data, if 0, is a const ref */
    const char *str;
} exr_attr_string_t;

/** Initializes storage for a string of the provided length
 *
 * This function assumes the string is uninitialized, so make sure use
 * attr_string_destroy to free any string prior to calling init
 */
EXR_EXPORT int exr_attr_string_init(
    exr_file_t *f, exr_attr_string_t *s, int32_t length );

/** Initializes a string with a static string (will not be freed)
 *
 * NB: As a performance optimization, no extra validation of length is
 * performed other than ensuring it is >= 0
 *
 * This function assumes the string is uninitialized, so make sure use
 * attr_string_destroy to free any string prior to calling init
 */
EXR_EXPORT int exr_attr_string_init_static_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *v, int32_t length );

/** Initializes a string with a static string (will not be freed).
 *
 * passes through to attr_string_init_static_with_length
 */
EXR_EXPORT int exr_attr_string_init_static(
    exr_file_t *f, exr_attr_string_t *s, const char *v );

/** Initializes and assigns a string value to the string with a precomputed length
 *
 * This function assumes the string is uninitialized, so make sure use
 * attr_string_destroy to free any string prior to calling init
 */
EXR_EXPORT int exr_attr_string_create_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *v, int32_t length );
/** Initializes and assigns a string value to the string
 *
 * This function assumes the string is uninitialized, so make sure use
 * attr_string_destroy to free any string prior to calling init
 */
EXR_EXPORT int exr_attr_string_create(
    exr_file_t *f, exr_attr_string_t *s, const char *v );

/** Assigns a string value to the string given a precomputed length, potentially resizing it */
EXR_EXPORT int exr_attr_string_set_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *v, int32_t length );

/** Assigns a string value to the string, potentially resizing it */
EXR_EXPORT int exr_attr_string_set(
    exr_file_t *f, exr_attr_string_t *s, const char *v );

/** Frees any owned memory associated with the string */
EXR_EXPORT void exr_attr_string_destroy( exr_attr_string_t *s );
    
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_STRING_H */
