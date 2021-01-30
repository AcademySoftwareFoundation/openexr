/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_STRUCTS_H
#define OPENEXR_PRIVATE_STRUCTS_H

#include "openexr.h"

#if defined __has_include
#  if __has_include (<stdatomic.h>)
#    define EXR_HAS_STD_ATOMICS 1
#  endif
#endif

#ifdef EXR_HAS_STD_ATOMICS
#  include <stdatomic.h>
#elif defined(_MSC_VER)

/* msvc w/ c11 support is only very new, until we know what the preprocessor checks are, provide defaults */
#  include <windows.h>

/* yeah, yeah, 32-bit is possible here, but if we make it the same, we
 * can write less since we know support is coming (eventually) */
typedef uint64_t atomic_uintptr_t;
typedef int64_t atomic_int_least64_t;

#define atomic_load(object) InterlockedOr64( (int64_t volatile *)object, 0 )
#define atomic_fetch_add(object, val) InterlockedExchangeAdd64( (int64_t volatile *)object, val )

static inline int atomic_compare_exchange_strong64( int64_t volatile *object, int64_t *expected, int64_t desired )
{
    int64_t prev = InterlockedCompareExchange64( object, desired, *expected );
    if ( prev == *expected ) 
        return 1;
    *expected = prev;
    return 0;
}
#define atomic_compare_exchange_strong(object, val, des)                \
    ( ( sizeof(*object) == 8 )                                          \
      ? atomic_compare_exchange_strong64( (int64_t volatile *)object, (int64_t *)val, (int64_t)des ) \
      : 0 )

#else
# error OS unimplemented support for atomics
#endif

typedef struct exr_part_t
{
    int part_index;
	exr_STORAGE_TYPE_t storage_mode; /**< Part of the file version flag declaring scanlines or tiled mode */

    exr_attribute_list_t attributes;

    /*  required attributes  */
    exr_attribute_t *channels;
    exr_attribute_t *compression;
    exr_attribute_t *dataWindow;
    exr_attribute_t *displayWindow;
    exr_attribute_t *lineOrder;
    exr_attribute_t *pixelAspectRatio;
    exr_attribute_t *screenWindowCenter;
    exr_attribute_t *screenWindowWidth;

    /** required for tiled files (@see storage_mode) */
    exr_attribute_t *tiles;

    /** required for deep or multipart files */
    exr_attribute_t *name;
    exr_attribute_t *type;
    exr_attribute_t *version;
    exr_attribute_t *chunkCount;
    /* in the file layout doc, but not required any more? */
    exr_attribute_t *maxSamplesPerPixel;

    /* copy commonly accessed required attributes to local struct memory for quick access */
    exr_attr_box2i_t data_window;
    exr_attr_box2i_t display_window;
    exr_COMPRESSION_TYPE_t comp_type;
    exr_LINEORDER_TYPE_t lineorder;

    int32_t num_tile_levels_x;
    int32_t num_tile_levels_y;
    int32_t *tile_level_tile_count_x;
    int32_t *tile_level_tile_count_y;
    int32_t *tile_level_tile_size_x;
    int32_t *tile_level_tile_size_y;

    uint64_t unpacked_size_per_chunk;
    int32_t lines_per_chunk;

    int32_t chunk_count;
    uint64_t chunk_table_offset;
    atomic_uintptr_t chunk_table;
} exr_PRIV_PART_t;

typedef enum
{
    EXR_MUST_READ_ALL = 0,
    EXR_ALLOW_SHORT_READ = 1
} __PRIV_READ_MODE;

typedef struct _priv_exr_file_t
{
    exr_attr_string_t filename;
    exr_attr_string_t tmp_filename;

    int (*do_read)( struct _priv_exr_file_t *file, void *, uint64_t, uint64_t *, int64_t *, __PRIV_READ_MODE );
    int (*do_write)( struct _priv_exr_file_t *file, const void *, uint64_t, uint64_t * );

    int (*standard_error)( struct _priv_exr_file_t *file, int code );
    int (*report_error)( struct _priv_exr_file_t *file, int code, const char *msg );
    int (*print_error)( struct _priv_exr_file_t *file, int code, const char *msg, ... ) EXR_PRINTF_FUNC_ATTRIBUTE;
    exr_error_handler_cb_t error_cb;

    void *user_data;
    exr_destroy_stream_func_ptr_t  destroy_fn;
    exr_read_func_ptr_t  read_fn;
    exr_write_func_ptr_t write_fn;

    atomic_int_least64_t file_offset; /**< used when writing, is there a better way? */
    int64_t file_size;

    uint8_t version;
    uint8_t max_name_length;

    uint8_t is_new_version:1;
    uint8_t is_singlepart_tiled:1;
    uint8_t has_nonimage_data:1;
    uint8_t is_multipart:1;

    exr_attribute_list_t custom_handlers;

    /** all files have at least one part */
    int num_parts;
    exr_PRIV_PART_t first_part;
    /* cheap array of one */
    exr_PRIV_PART_t *init_part;
    exr_PRIV_PART_t **parts;
} exr_PRIV_FILE_t;
#define EXR_GETFILE(f) ((exr_PRIV_FILE_t *) (f))

int priv_add_part( exr_PRIV_FILE_t *, exr_PRIV_PART_t ** );

int priv_create_file( exr_PRIV_FILE_t **, exr_error_handler_cb_t errcb, uint64_t userdatasz, int isread );
void priv_destroy_file( exr_PRIV_FILE_t * );

#endif /* OPENEXR_PRIVATE_STRUCTS_H */
