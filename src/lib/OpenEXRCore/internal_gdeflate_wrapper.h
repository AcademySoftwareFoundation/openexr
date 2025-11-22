/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_GDEFLATE_WRAPPER_H
#define OPENEXR_CORE_GDEFLATE_WRAPPER_H

/*
 * GDEFLATE SUPPORT WRAPPERS
 *
 * Conditional compilation for gdeflate support is isolated here.
 * OPENEXR_ENABLE_GDEFLATE is set by CMake based on libdeflate detection.
 *
 * Note: This header must be included after libdeflate.h (or internal deflate
 * sources) are already included, as it depends on libdeflate types.
 */

#include "internal_memory.h"
#include "openexr_context.h"
#include "openexr_errors.h"

/* libdeflate types already available via compression.c includes */

#ifndef OPENEXR_ENABLE_GDEFLATE
/* Stub types when gdeflate not available */
struct libdeflate_gdeflate_compressor
{
    int unused;
};
struct libdeflate_gdeflate_decompressor
{
    int unused;
};
struct libdeflate_gdeflate_out_page
{
    void*  data;
    size_t nbytes;
};
struct libdeflate_gdeflate_in_page
{
    const void* data;
    size_t      nbytes;
};
#endif

static inline size_t
wrap_gdeflate_compress_bound (
    void* compressor, size_t in_bytes, uint64_t* out_page_count)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    size_t page_count;
    size_t result = libdeflate_gdeflate_compress_bound (
        compressor, in_bytes, &page_count);
    *out_page_count = page_count;
    return result;
#else
    (void) compressor;
    (void) in_bytes;
    *out_page_count = 0;
    return 0;
#endif
}

static inline exr_result_t
wrap_alloc_gdeflate_compressor (
    int level,
    exr_const_context_t ctxt,
    struct libdeflate_gdeflate_compressor** out_comp)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    /* Note: gdeflate does not have _ex() allocator variants, so we always use
     * libdeflate_set_memory_allocator() regardless of libdeflate version */
    libdeflate_set_memory_allocator (
        ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        ctxt ? ctxt->free_fn : internal_exr_free);
    *out_comp = libdeflate_alloc_gdeflate_compressor (level);
    return *out_comp ? EXR_ERR_SUCCESS : EXR_ERR_OUT_OF_MEMORY;
#else
    (void) level;
    (void) ctxt;
    *out_comp = NULL;
    return EXR_ERR_FEATURE_NOT_IMPLEMENTED;
#endif
}

static inline void
wrap_free_gdeflate_compressor (struct libdeflate_gdeflate_compressor* comp)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    libdeflate_free_gdeflate_compressor (comp);
#else
    (void) comp;
#endif
}

static inline size_t
wrap_gdeflate_compress (
    struct libdeflate_gdeflate_compressor*  comp,
    const void*                             in,
    size_t                                  in_bytes,
    struct libdeflate_gdeflate_out_page*    out_pages,
    size_t                                  out_page_count)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    return libdeflate_gdeflate_compress (
        comp, in, in_bytes, out_pages, out_page_count);
#else
    (void) comp;
    (void) in;
    (void) in_bytes;
    (void) out_pages;
    (void) out_page_count;
    return 0;
#endif
}

static inline exr_result_t
wrap_alloc_gdeflate_decompressor (
    exr_const_context_t ctxt,
    struct libdeflate_gdeflate_decompressor** out_decomp)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    /* Note: gdeflate does not have _ex() allocator variants, so we always use
     * libdeflate_set_memory_allocator() regardless of libdeflate version */
    libdeflate_set_memory_allocator (
        ctxt ? ctxt->alloc_fn : internal_exr_alloc,
        ctxt ? ctxt->free_fn : internal_exr_free);
    *out_decomp = libdeflate_alloc_gdeflate_decompressor ();
    return *out_decomp ? EXR_ERR_SUCCESS : EXR_ERR_OUT_OF_MEMORY;
#else
    (void) ctxt;
    *out_decomp = NULL;
    return EXR_ERR_FEATURE_NOT_IMPLEMENTED;
#endif
}

static inline void
wrap_free_gdeflate_decompressor (
    struct libdeflate_gdeflate_decompressor* decomp)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    libdeflate_free_gdeflate_decompressor (decomp);
#else
    (void) decomp;
#endif
}

static inline enum libdeflate_result
wrap_gdeflate_decompress (
    struct libdeflate_gdeflate_decompressor* decomp,
    struct libdeflate_gdeflate_in_page*    in_pages,
    size_t                                 in_page_count,
    void*                                  out,
    size_t                                 out_bytes_avail,
    size_t*                                actual_out)
{
#ifdef OPENEXR_ENABLE_GDEFLATE
    return libdeflate_gdeflate_decompress (
        decomp, in_pages, in_page_count, out, out_bytes_avail, actual_out);
#else
    (void) decomp;
    (void) in_pages;
    (void) in_page_count;
    (void) out;
    (void) out_bytes_avail;
    (void) actual_out;
    return LIBDEFLATE_BAD_DATA;
#endif
}

#endif /* OPENEXR_CORE_GDEFLATE_WRAPPER_H */

