/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/


#include "openexr_file_info.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

const char *exr_get_file_name( exr_file_t *file )
{
    exr_PRIV_FILE_t *pf = EXR_GETFILE(file);
    if ( pf )
        return pf->filename.str;
    return NULL;
}

/**************************************/

int exr_get_part_count( exr_file_t *file )
{
    if ( file )
        return EXR_GETFILE(file)->num_parts;
    return 0;
}

/**************************************/

int exr_has_deep( exr_file_t *file )
{
    if ( file )
        return EXR_GETFILE(file)->has_nonimage_data;
    return 0;
}

/**************************************/

const char * exr_get_part_name(
    exr_file_t *file,
    int part_index )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        exr_attribute_t *name = f->parts[part_index]->name;
        if ( name )
            return name->string->str;
    }
    return NULL;
}

/**************************************/

exr_STORAGE_TYPE_t exr_get_part_storage(
    exr_file_t *file,
    int part_index )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return f->parts[part_index]->storage_mode;
    }
    return EXR_STORAGE_LAST_TYPE;
}

/**************************************/

int exr_get_tile_levels( exr_file_t *f, int part_index, int *levelsx, int *levelsy )
{
    exr_PRIV_PART_t *part;
    exr_PRIV_FILE_t *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
        return EXR_GETFILE(f)->standard_error( f, EXR_ERR_INVALID_ARGUMENT );

    part = file->parts[part_index];
    if ( part->storage_mode == EXR_STORAGE_TILED ||
         part->storage_mode == EXR_STORAGE_DEEP_TILED )
    {
        if ( levelsx )
            *levelsx = part->num_tile_levels_x;
        if ( levelsy )
            *levelsy = part->num_tile_levels_y;
        return EXR_ERR_SUCCESS;
    }

    return EXR_GETFILE(f)->standard_error( f, EXR_ERR_TILE_SCAN_MIXEDAPI );
}

/**************************************/

int exr_get_chunk_count( exr_file_t *f, int part_index )
{
    exr_PRIV_PART_t *part;
    exr_PRIV_FILE_t *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
        return -1;

    part = file->parts[part_index];
    if ( part->dataWindow )
    {
        if ( part->storage_mode == EXR_STORAGE_TILED ||
             part->storage_mode == EXR_STORAGE_DEEP_TILED )
        {
            if ( part->tiles )
                return part->chunk_count;
        }
        else if ( part->storage_mode == EXR_STORAGE_SCANLINE ||
                  part->storage_mode == EXR_STORAGE_DEEP_SCANLINE )
        {
            if ( part->compression )
                return part->chunk_count;
        }
    }
            
    return -1;
}

/**************************************/

int32_t exr_get_scanlines_per_chunk( exr_file_t *f, int part_index )
{
    exr_PRIV_PART_t *part;
    exr_PRIV_FILE_t *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
    {
        EXR_GETFILE(f)->print_error( f, EXR_ERR_INVALID_ARGUMENT,
                                     "Invalid part number (%d) in request to get_scanlines_per_chunk",
                                     part_index );
        return -1;
    }

    part = file->parts[part_index];
    if ( part->storage_mode == EXR_STORAGE_SCANLINE ||
         part->storage_mode == EXR_STORAGE_DEEP_SCANLINE )
    {
        return part->lines_per_chunk;
    }
    EXR_GETFILE(f)->print_error( f, EXR_ERR_INVALID_ARGUMENT,
                                 "Invalid part storage mode for (%d) for scanline information",
                                 (int)part->storage_mode );
    return -1;
}
