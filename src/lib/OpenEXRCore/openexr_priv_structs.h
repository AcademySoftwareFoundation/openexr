/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_STRUCTS_H
#define OPENEXR_PRIVATE_STRUCTS_H

#include "openexr.h"

#include <stdatomic.h>

typedef struct EXR_TYPE(part)
{
    int part_index;
	EXR_TYPE(STORAGE_TYPE) storage_mode; /**< Part of the file version flag declaring scanlines or tiled mode */

    EXR_TYPE(attribute_list) attributes;

    /*  required attributes  */
    EXR_TYPE(attribute) *channels;
    EXR_TYPE(attribute) *compression;
    EXR_TYPE(attribute) *dataWindow;
    EXR_TYPE(attribute) *displayWindow;
    EXR_TYPE(attribute) *lineOrder;
    EXR_TYPE(attribute) *pixelAspectRatio;
    EXR_TYPE(attribute) *screenWindowCenter;
    EXR_TYPE(attribute) *screenWindowWidth;

    /** required for tiled files (@see storage_mode) */
    EXR_TYPE(attribute) *tiles;

    /** required for deep or multipart files */
    EXR_TYPE(attribute) *name;
    EXR_TYPE(attribute) *type;
    EXR_TYPE(attribute) *version;
    EXR_TYPE(attribute) *chunkCount;
    /* in the file layout doc, but not required any more? */
    EXR_TYPE(attribute) *maxSamplesPerPixel;

    /* copy commonly accessed required attributes to local struct memory for quick access */
    EXR_TYPE(attr_box2i) data_window;
    EXR_TYPE(attr_box2i) display_window;
    EXR_TYPE(COMPRESSION_TYPE) comp_type;
    EXR_TYPE(LINEORDER_TYPE) lineorder;

    int32_t num_tile_levels_x;
    int32_t num_tile_levels_y;
    int32_t *tile_level_tile_count_x;
    int32_t *tile_level_tile_count_y;
    int32_t *tile_level_tile_size_x;
    int32_t *tile_level_tile_size_y;

    size_t unpacked_size_per_chunk;
    int32_t lines_per_chunk;

    int32_t chunk_count;
    off_t chunk_table_offset;
    atomic_uintptr_t chunk_table;
} EXR_TYPE(PRIV_PART);

typedef enum
{
    EXR_MUST_READ_ALL = 0,
    EXR_ALLOW_SHORT_READ = 1
} __PRIV_READ_MODE;

typedef struct EXR_TYPE(file)
{
    EXR_TYPE(attr_string) filename;
    EXR_TYPE(attr_string) tmp_filename;

    int (*do_read)( struct EXR_TYPE(file) *file, void *, size_t, off_t *, ssize_t *, __PRIV_READ_MODE );
    int (*do_write)( struct EXR_TYPE(file) *file, const void *, size_t, off_t * );

    int (*standard_error)( struct EXR_TYPE(file) *file, int code );
    int (*report_error)( struct EXR_TYPE(file) *file, int code, const char *msg );
    int (*print_error)( struct EXR_TYPE(file) *file, int code, const char *msg, ... ) EXR_PRINTF_FUNC_ATTRIBUTE;
    EXR_TYPE(error_handler_cb) error_cb;

    void *user_data;
    EXR_TYPE(destroy_stream_func_ptr)  destroy_fn;
    EXR_TYPE(read_func_ptr)  read_fn;
    EXR_TYPE(write_func_ptr) write_fn;

    atomic_long file_offset; /**< used when writing */
    ssize_t file_size;

    uint8_t version;
    uint8_t max_name_length;

    uint8_t is_new_version:1;
    uint8_t is_singlepart_tiled:1;
    uint8_t has_nonimage_data:1;
    uint8_t is_multipart:1;

    EXR_TYPE(attribute_list) custom_handlers;

    /** all files have at least one part */
    int num_parts;
    EXR_TYPE(PRIV_PART) first_part;
    /* cheap array of one */
    EXR_TYPE(PRIV_PART) *init_part;
    EXR_TYPE(PRIV_PART) **parts;
} EXR_TYPE(PRIV_FILE);
#define EXR_GETFILE(f) ((EXR_TYPE(PRIV_FILE) *) (f))

int priv_add_part( EXR_TYPE(PRIV_FILE) *, EXR_TYPE(PRIV_PART) ** );

int priv_create_file( EXR_TYPE(PRIV_FILE) **, EXR_TYPE(error_handler_cb) errcb, size_t userdatasz, int isread );
void priv_destroy_file( EXR_TYPE(PRIV_FILE) * );

#endif /* OPENEXR_PRIVATE_STRUCTS_H */
