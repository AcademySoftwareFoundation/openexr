/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_STRUCTS_H
#define OPENEXR_PRIVATE_STRUCTS_H

#include "internal_attr.h"

/* for testing, we include a bunch of internal stuff into the unit tests which are in c++ */
#ifdef __cplusplus
#    include <atomic>
using atomic_uintptr_t     = std::atomic_uintptr_t;
using atomic_int_least64_t = std::atomic_int_least64_t;
inline int64_t
atomic_load (const atomic_int_least64_t* v)
{
    return v->load ();
}
inline uintptr_t
atomic_load (const atomic_uintptr_t* v)
{
    return v->load ();
}
inline int
atomic_compare_exchange_strong (
    atomic_int_least64_t* obj, int64_t* expected, int64_t desired)
{
    return obj->compare_exchange_strong (*expected, desired) ? 1 : 0;
}
inline int
atomic_compare_exchange_strong (
    atomic_uintptr_t* obj, uintptr_t* expected, uintptr_t desired)
{
    return obj->compare_exchange_strong (*expected, desired) ? 1 : 0;
}
#else
#    if defined __has_include
#        if __has_include(<stdatomic.h>)
#            define EXR_HAS_STD_ATOMICS 1
#        endif
#    endif

#    ifdef EXR_HAS_STD_ATOMICS
#        include <stdatomic.h>
#    elif defined(_MSC_VER)

/* msvc w/ c11 support is only very new, until we know what the preprocessor checks are, provide defaults */
#        include <windows.h>

/* yeah, yeah, 32-bit is possible here, but if we make it the same, we
 * can write less since we know support is coming (eventually) */
typedef uint64_t atomic_uintptr_t;
typedef int64_t  atomic_int_least64_t;

#        define atomic_load(object)                                            \
            InterlockedOr64 ((int64_t volatile*) object, 0)
#        define atomic_fetch_add(object, val)                                  \
            InterlockedExchangeAdd64 ((int64_t volatile*) object, val)

static inline int
atomic_compare_exchange_strong64 (
    int64_t volatile* object, int64_t* expected, int64_t desired)
{
    int64_t prev = InterlockedCompareExchange64 (object, desired, *expected);
    if (prev == *expected) return 1;
    *expected = prev;
    return 0;
}
#        define atomic_compare_exchange_strong(object, val, des)               \
            ((sizeof (*object) == 8) ? atomic_compare_exchange_strong64 (      \
                                           (int64_t volatile*) object,         \
                                           (int64_t*) val,                     \
                                           (int64_t) des)                      \
                                     : 0)

#    else
#        error OS unimplemented support for atomics
#    endif
#endif

struct _internal_exr_part
{
    int part_index;
    exr_storage_t
        storage_mode; /**< Part of the file version flag declaring scanlines or tiled mode */

    exr_attribute_list_t attributes;

    /*  required attributes  */
    exr_attribute_t* channels;
    exr_attribute_t* compression;
    exr_attribute_t* dataWindow;
    exr_attribute_t* displayWindow;
    exr_attribute_t* lineOrder;
    exr_attribute_t* pixelAspectRatio;
    exr_attribute_t* screenWindowCenter;
    exr_attribute_t* screenWindowWidth;

    /** required for tiled files (@see storage_mode) */
    exr_attribute_t* tiles;

    /** required for deep or multipart files */
    exr_attribute_t* name;
    exr_attribute_t* type;
    exr_attribute_t* version;
    exr_attribute_t* chunkCount;
    /* in the file layout doc, but not required any more? */
    exr_attribute_t* maxSamplesPerPixel;

    /* copy commonly accessed required attributes to local struct memory for quick access */
    exr_attr_box2i_t  data_window;
    exr_attr_box2i_t  display_window;
    exr_compression_t comp_type;
    exr_lineorder_t   lineorder;

    int32_t  num_tile_levels_x;
    int32_t  num_tile_levels_y;
    int32_t* tile_level_tile_count_x;
    int32_t* tile_level_tile_count_y;
    int32_t* tile_level_tile_size_x;
    int32_t* tile_level_tile_size_y;

    uint64_t unpacked_size_per_chunk;
    int32_t  lines_per_chunk;

    int32_t          chunk_count;
    uint64_t         chunk_table_offset;
    atomic_uintptr_t chunk_table;
};

enum _INTERNAL_EXR_READ_MODE
{
    EXR_MUST_READ_ALL    = 0,
    EXR_ALLOW_SHORT_READ = 1
};

enum _INTERNAL_EXR_CONTEXT_MODE
{
    EXR_CONTEXT_READ          = 0,
    EXR_CONTEXT_WRITE         = 1,
    EXR_CONTEXT_UPDATE_HEADER = 2,
    EXR_CONTEXT_WRITING_DATA  = 3,
    EXR_CONTEXT_WRITE_FINISHED
};

struct _internal_exr_context
{
    uint8_t mode;
    uint8_t version;
    uint8_t max_name_length;

    uint8_t is_new_version : 1;
    uint8_t is_singlepart_tiled : 1;
    uint8_t has_nonimage_data : 1;
    uint8_t is_multipart : 1;

    exr_attr_string_t filename;
    exr_attr_string_t tmp_filename;

    exr_result_t (*do_read) (
        const struct _internal_exr_context* file,
        void*,
        uint64_t,
        uint64_t*,
        int64_t*,
        enum _INTERNAL_EXR_READ_MODE);
    exr_result_t (*do_write) (
        struct _internal_exr_context* file, const void*, uint64_t, uint64_t*);

    exr_result_t (*standard_error) (
        const exr_context_t ctxt, exr_result_t code);
    exr_result_t (*report_error) (
        const exr_context_t ctxt, exr_result_t code, const char* msg);
    exr_result_t (*print_error) (
        const exr_context_t ctxt, exr_result_t code, const char* msg, ...)
        EXR_PRINTF_FUNC_ATTRIBUTE;
    exr_error_handler_cb_t error_handler_fn;

    exr_memory_allocation_func_t alloc_fn;
    exr_memory_free_func_t       free_fn;

    int max_image_w;
    int max_image_h;
    int max_tile_w;
    int max_tile_h;

    void*                         user_data;
    exr_destroy_stream_func_ptr_t destroy_fn;

    int64_t             file_size;
    exr_read_func_ptr_t read_fn;

    exr_write_func_ptr_t write_fn;
    atomic_int_least64_t
        file_offset; /**< used when writing, is there a better way? */

    exr_attribute_list_t custom_handlers;

    /** all files have at least one part */
    int                       num_parts;
    struct _internal_exr_part first_part;
    /* cheap array of one */
    struct _internal_exr_part*  init_part;
    struct _internal_exr_part** parts;
};

#define EXR_CTXT(c) ((struct _internal_exr_context*) (c))
#define EXR_CCTXT(c) ((const struct _internal_exr_context*) (c))
#define EXR_PROMOTE_CONTEXT_OR_ERROR(c)                                        \
    struct _internal_exr_context* pctxt = EXR_CTXT (c);                        \
    if (!pctxt) return EXR_ERR_MISSING_CONTEXT_ARG

#define EXR_PROMOTE_CONST_CONTEXT_OR_ERROR(c)                                  \
    const struct _internal_exr_context* pctxt = EXR_CCTXT (c);                 \
    if (!pctxt) return EXR_ERR_MISSING_CONTEXT_ARG

#define EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR(c, pi)                           \
    struct _internal_exr_context* pctxt = EXR_CTXT (c);                        \
    struct _internal_exr_part*    part;                                        \
    if (!pctxt) return EXR_ERR_MISSING_CONTEXT_ARG;                            \
    if (pi < 0 || pi >= pctxt->num_parts)                                      \
        return pctxt->print_error (                                            \
            c,                                                                 \
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                     \
            "Part index (%d) out of range",                                    \
            pi);                                                               \
    part = pctxt->parts[pi]

#define EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR(c, pi)                     \
    const struct _internal_exr_context* pctxt = EXR_CCTXT (c);                 \
    const struct _internal_exr_part*    part;                                  \
    if (!pctxt) return EXR_ERR_MISSING_CONTEXT_ARG;                            \
    if (pi < 0 || pi >= pctxt->num_parts)                                      \
        return pctxt->print_error (                                            \
            c,                                                                 \
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,                                     \
            "Part index (%d) out of range",                                    \
            pi);                                                               \
    part = pctxt->parts[pi]

#define EXR_VALIDATE_PART_IDX_OR_ERROR(pi)                                     \
    if (pi < 0 || pi >= pctxt->num_parts)                                      \
    return pctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE)

void internal_exr_update_default_handlers (exr_context_initializer_t* inits);

exr_result_t internal_exr_add_part (
    struct _internal_exr_context*, struct _internal_exr_part**);

int internal_exr_add_part (
    struct _internal_exr_context*, struct _internal_exr_part**);
exr_result_t internal_exr_alloc_context (
    struct _internal_exr_context**   out,
    const exr_context_initializer_t* initializers,
    enum _INTERNAL_EXR_CONTEXT_MODE  mode,
    size_t                           extra_data);
void internal_exr_destroy_context (struct _internal_exr_context* ctxt);

#endif /* OPENEXR_PRIVATE_STRUCTS_H */
