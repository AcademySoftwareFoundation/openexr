/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_MEMORY_H
#define OPENEXR_PRIVATE_MEMORY_H

#include "openexr_base.h"

#if defined(__GNUC__) || defined(__clang__)
__attribute__((malloc))
#endif
void *internal_exr_alloc( size_t bytes );

void internal_exr_free( void *ptr );

#if defined(__GNUC__) || defined(__clang__)
__attribute__ ((malloc))
#endif
void*
internal_exr_alloc_aligned (void **tofreeptr, size_t bytes, size_t align);

#endif /* OPENEXR_PRIVATE_MEMORY_H */
