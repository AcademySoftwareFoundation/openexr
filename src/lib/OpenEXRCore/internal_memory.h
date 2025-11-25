/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_MEMORY_H
#define OPENEXR_PRIVATE_MEMORY_H

#include "openexr_base.h"

#include "OpenEXRConfig.h"

OPENEXR_CORE_NAMESPACE_ENTER

#if defined(__GNUC__) || defined(__clang__)
__attribute__ ((malloc))
#endif
#if __cplusplus && defined(OPENEXR_CORE_USE_NAMESPACE)
EXR_EXPORT
#endif
void*
internal_exr_alloc (size_t bytes);

#if defined(__GNUC__) || defined(__clang__)
__attribute__ ((malloc))
#endif
#if __cplusplus && defined(OPENEXR_CORE_USE_NAMESPACE)
EXR_EXPORT
#endif
void*
internal_exr_alloc_aligned (
    void* (*alloc_fn) (size_t), void** tofreeptr, size_t bytes, size_t align);

#if __cplusplus && defined(OPENEXR_CORE_USE_NAMESPACE)
EXR_EXPORT
#endif
void internal_exr_free (void* ptr);

OPENEXR_CORE_NAMESPACE_EXIT

#endif /* OPENEXR_PRIVATE_MEMORY_H */
