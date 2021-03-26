/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/


#include "openexr_file_info.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

int exr_attr_count( exr_file_t *file, int part_index )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return f->parts[part_index]->attributes.num_attributes;
    }
    return -1;
}

/**************************************/

exr_attribute_t *exr_attr_find_by_name(
    exr_file_t *file, int part_index, const char *name )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return exr_attr_list_find_by_name(
            file, &(f->parts[part_index]->attributes), name );
    }
    return NULL;
}

/**************************************/

exr_attribute_list_t *exr_get_attribute_list(
    exr_file_t *file, int part_index )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return &(f->parts[part_index]->attributes);
    }
    return NULL;
}

/**************************************/

exr_result_t exr_attr_declare_by_type(
    exr_file_t *file,
    int part_index,
    const char *name,
    const char *type,
    exr_attribute_t **outattr )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    exr_PRIV_PART_t *part;
    if ( ! file )
        return EXR_ERR_INVALID_ARGUMENT;

    if ( part_index < 0 || part_index >= f->num_parts )
    {
        return f->print_error(
            file, EXR_ERR_INVALID_ARGUMENT,
            "Invalid part index (%d) requested",
            part_index );
    }

    part = f->parts[part_index];
    return exr_attr_list_add_by_type(
        file, &(part->attributes), name, type, 0, NULL, outattr );
}

/**************************************/

exr_result_t exr_attr_declare(
    exr_file_t *file,
    int part_index,
    const char *name,
    exr_ATTRIBUTE_TYPE_t type,
    exr_attribute_t **outattr )
{
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);
    exr_PRIV_PART_t *part;
    if ( ! file )
        return EXR_ERR_INVALID_ARGUMENT;

    if ( part_index < 0 || part_index >= f->num_parts )
    {
        return f->print_error(
            file, EXR_ERR_INVALID_ARGUMENT,
            "Invalid part index (%d) requested",
            part_index );
    }

    part = f->parts[part_index];
    return exr_attr_list_add(
        file, &(part->attributes), name, type, 0, NULL, outattr );
}

