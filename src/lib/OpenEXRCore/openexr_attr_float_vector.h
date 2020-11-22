/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_FLOAT_VECTOR_H
#define OPENEXR_ATTR_FLOAT_VECTOR_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup float vector attribute storage and functions
 * @brief These are a group of functions for handling a sized list of floats
 * @{
 */

/** float vector storage struct */
typedef struct
{
    int32_t length;
    int32_t alloc_size; /**< if this is non-zero, the float vector owns the data, if 0, is a const ref */
    const float *arr;
} EXR_TYPE(attr_float_vector);

/** Allocates storage for a float vector with the provided number of entries.
 *
 * Leaves the float data uninitialized
 */
EXR_EXPORT int EXR_FUN(attr_float_vector_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, int32_t nent );
/** Initializes a float vector with the provided number of entries and sets the pointer to the provided pointer
 *
 * This will result in a float vector pointing at a float array that
 * is owned by the calling application and will not be freed, and is
 * expected to outlive the lifetime of the attribute.
 */
EXR_EXPORT int EXR_FUN(attr_float_vector_init_static)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, const float *arr, int32_t nent );
/** Allocates storage for a float vector with the provided number of entries and initializes */
EXR_EXPORT int EXR_FUN(attr_float_vector_create)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, const float *arr, int32_t nent );

/** Frees any owned storage for a float vector */
EXR_EXPORT void EXR_FUN(attr_float_vector_destroy)( EXR_TYPE(attr_float_vector) *fv );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_FLOAT_VECTOR_H */
