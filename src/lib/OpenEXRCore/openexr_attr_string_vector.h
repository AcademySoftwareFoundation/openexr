/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_STRING_VECTOR_H
#define OPENEXR_ATTR_STRING_VECTOR_H

#include "openexr_attr_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup string vector attribute storage and functions
 * @brief These are a group of functions for handling a list of strings and associated data
 * @{
 */

/** storage for a string vector */
typedef struct
{
    int32_t n_strings;
    int32_t alloc_size; /**< if this is non-zero, the string vector owns the data, if 0, is a const ref */
    const EXR_TYPE(attr_string) *strings;
} EXR_TYPE(attr_string_vector);

/** @brief Allocates memory for a list of strings of length nelt
 *
 * This presumes the attr_string_vector passed in is uninitialized prior to this call
 *
 * @param f file for associated string vector (used for error reporting)
 * @param sv pointer to attribute to initialize. Assumed uninitialized
 * @param nelt desired size of string vector
 *
 * @return 0 on success, error code otherwise
 */
EXR_EXPORT int EXR_FUN(attr_string_vector_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t nelt );

/** @brief Frees memory for the channel list and all channels inside */
EXR_EXPORT void EXR_FUN(attr_string_vector_destroy)( EXR_TYPE(attr_string_vector) *sv );

/** @brief Allocates memory for a particular string within the list
 *
 * This enables one to pre-allocate, then read directly into the string
 *
 * @param f file for associated string vector (used for error reporting)
 * @param sv pointer to string vector. It should have been resized ahead of calling
 * @param idx index of the string to initialize
 * @param length desired size of string 
 *
 * @return 0 on success, error code otherwise
 */
EXR_EXPORT int EXR_FUN(attr_string_vector_init_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, int32_t length );

/** @brief Set a string within the string vector */
EXR_EXPORT int EXR_FUN(attr_string_vector_set_entry_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, const char *s, int32_t length );
/** @brief Set a string within the string vector */
EXR_EXPORT int EXR_FUN(attr_string_vector_set_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, const char *s );

/** @brief Append a string to the string vector */
EXR_EXPORT int EXR_FUN(attr_string_vector_add_entry_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, const char *s, int32_t length );
/** @brief Append a string to the string vector */
EXR_EXPORT int EXR_FUN(attr_string_vector_add_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, const char *s );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_STRING_VECTOR_H */
