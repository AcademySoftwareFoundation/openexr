/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/


#include "openexr_file_info.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

const char *EXR_FUN(get_file_name)( EXR_TYPE(FILE) *file )
{
    EXR_TYPE(PRIV_FILE) *pf = EXR_GETFILE(file);
    if ( pf )
        return pf->filename.str;
    return NULL;
}

/**************************************/

int EXR_FUN(get_part_count)( EXR_TYPE(FILE) *file )
{
    if ( file )
        return EXR_GETFILE(file)->num_parts;
    return 0;
}

/**************************************/

int EXR_FUN(has_deep)( EXR_TYPE(FILE) *file )
{
    if ( file )
        return EXR_GETFILE(file)->has_nonimage_data;
    return 0;
}

/**************************************/

const char * EXR_FUN(get_part_name)(
    EXR_TYPE(FILE) *file,
    int part_index )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        EXR_TYPE(attribute) *name = f->parts[part_index]->name;
        if ( name )
            return name->string->str;
    }
    return NULL;
}

/**************************************/

EXR_TYPE(STORAGE_TYPE) EXR_FUN(get_part_storage)(
    EXR_TYPE(FILE) *file,
    int part_index )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return f->parts[part_index]->storage_mode;
    }
    return EXR_DEF(STORAGE_LAST_TYPE);
}

/**************************************/

int EXR_FUN(get_tile_levels)( EXR_TYPE(FILE) *f, int part_index, int *levelsx, int *levelsy )
{
    EXR_TYPE(PRIV_PART) *part;
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
        return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_INVALID_ARGUMENT) );

    part = file->parts[part_index];
    if ( part->storage_mode == EXR_DEF(STORAGE_TILED) ||
         part->storage_mode == EXR_DEF(STORAGE_DEEP_TILED) )
    {
        if ( levelsx )
            *levelsx = part->num_tile_levels_x;
        if ( levelsy )
            *levelsy = part->num_tile_levels_y;
        return EXR_DEF(ERR_SUCCESS);
    }

    return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_TILE_SCAN_MIXEDAPI) );
}

/**************************************/

int EXR_FUN(get_chunk_count)( EXR_TYPE(FILE) *f, int part_index )
{
    EXR_TYPE(PRIV_PART) *part;
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
        return -1;

    part = file->parts[part_index];
    if ( part->dataWindow )
    {
        if ( part->storage_mode == EXR_DEF(STORAGE_TILED) ||
             part->storage_mode == EXR_DEF(STORAGE_DEEP_TILED) )
        {
            if ( part->tiles )
                return part->chunk_count;
        }
        else if ( part->storage_mode == EXR_DEF(STORAGE_SCANLINE) ||
                  part->storage_mode == EXR_DEF(STORAGE_DEEP_SCANLINE) )
        {
            if ( part->compression )
                return part->chunk_count;
        }
    }
            
    return -1;
}

/**************************************/

int32_t EXR_FUN(get_scanlines_per_chunk)( EXR_TYPE(FILE) *f, int part_index )
{
    EXR_TYPE(PRIV_PART) *part;
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);

    if ( ! f )
        return -1;

    if ( part_index < 0 || part_index >= file->num_parts )
    {
        EXR_GETFILE(f)->print_error( f, EXR_DEF(ERR_INVALID_ARGUMENT),
                                     "Invalid part number (%d) in request to get_scanlines_per_chunk",
                                     part_index );
        return -1;
    }

    part = file->parts[part_index];
    if ( part->storage_mode == EXR_DEF(STORAGE_SCANLINE) ||
         part->storage_mode == EXR_DEF(STORAGE_DEEP_SCANLINE) )
    {
        return part->lines_per_chunk;
    }
    EXR_GETFILE(f)->print_error( f, EXR_DEF(ERR_INVALID_ARGUMENT),
                                 "Invalid part storage mode for (%d) for scanline information",
                                 (int)part->storage_mode );
    return -1;
}
