/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/


#include "openexr_file_info.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

int EXR_FUN(attr_count)( EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return f->parts[part_index]->attributes.num_attributes;
    }
    return -1;
}

/**************************************/

EXR_TYPE(attribute) *EXR_FUN(attr_find_by_name)(
    EXR_TYPE(FILE) *file, int part_index, const char *name )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return EXR_FUN(attr_list_find_by_name)(
            file, &(f->parts[part_index]->attributes), name );
    }
    return NULL;
}

/**************************************/

EXR_TYPE(attribute_list) *EXR_FUN(get_attribute_list)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    if ( f && part_index >= 0 && part_index < f->num_parts )
    {
        return &(f->parts[part_index]->attributes);
    }
    return NULL;
}

/**************************************/

int EXR_FUN(attr_declare_by_type)(
    EXR_TYPE(FILE) *file,
    int part_index,
    const char *name,
    const char *type,
    EXR_TYPE(attribute) **outattr )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    EXR_TYPE(PRIV_PART) *part;
    if ( ! file )
        return EXR_DEF(ERR_INVALID_ARGUMENT);

    if ( part_index < 0 || part_index >= f->num_parts )
    {
        return f->print_error(
            file, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Invalid part index (%d) requested",
            part_index );
    }

    part = f->parts[part_index];
    return EXR_FUN(attr_list_add_by_type)(
        file, &(part->attributes), name, type, 0, NULL, outattr );
}

/**************************************/

int EXR_FUN(attr_declare)(
    EXR_TYPE(FILE) *file,
    int part_index,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    EXR_TYPE(attribute) **outattr )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    EXR_TYPE(PRIV_PART) *part;
    if ( ! file )
        return EXR_DEF(ERR_INVALID_ARGUMENT);

    if ( part_index < 0 || part_index >= f->num_parts )
    {
        return f->print_error(
            file, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Invalid part index (%d) requested",
            part_index );
    }

    part = f->parts[part_index];
    return EXR_FUN(attr_list_add)(
        file, &(part->attributes), name, type, 0, NULL, outattr );
}

