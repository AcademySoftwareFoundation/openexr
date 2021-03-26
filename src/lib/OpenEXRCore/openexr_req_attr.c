/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_req_attr.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

#define REQ_ATTR_GET_IMPL(name, entry) \
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);                 \
    if ( f && part_index >= 0 && part_index < f->num_parts )    \
    {                                                           \
        exr_PRIV_PART_t *part = f->parts[part_index];       \
        if ( part->name )                                       \
            retval = part->name->entry;                         \
    }

#define REQ_ATTR_FIND_CREATE(name, type)                        \
    exr_PRIV_FILE_t *f = EXR_GETFILE(file);                 \
    if ( f && part_index >= 0 && part_index < f->num_parts )    \
    {                                                           \
        exr_PRIV_PART_t *part = f->parts[part_index];       \
        if ( part->name )                                       \
        {                                                       \
            attr = part->name;                                  \
            rv = EXR_ERR_SUCCESS;                          \
        }                                                       \
        else                                                    \
        {                                                       \
            rv = exr_attr_list_add(                        \
                file, &(part->attributes), #name,               \
                type, 0, NULL, &(part->name) );                 \
            if ( rv == EXR_ERR_SUCCESS )                   \
                attr = part->name;                              \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        rv = f->print_error(                                    \
            file, EXR_ERR_INVALID_ARGUMENT,                \
            "Missing file or invalid part number (%d)",         \
            part_index );                                       \
    }

/**************************************/

const exr_attr_chlist_t *exr_get_channels(
    exr_file_t *file, int part_index )
{
    exr_attr_chlist_t *retval = NULL;
    REQ_ATTR_GET_IMPL(channels, chlist);
    return retval;
}

/**************************************/

int exr_add_channels(
    exr_file_t *file, int part_index,
    const char *name,
    exr_PIXEL_TYPE_t ptype,
    uint8_t islinear,
    int32_t xsamp, int32_t ysamp )
{
    exr_attribute_t *attr = NULL;
    int rv;
    REQ_ATTR_FIND_CREATE(channels, EXR_ATTR_CHLIST );
    if ( rv == EXR_ERR_SUCCESS )
    {
        rv = exr_attr_chlist_add(
            file, attr->chlist, name, ptype, islinear, xsamp, ysamp );
    }
    return rv;
}

/**************************************/

int exr_set_channels(
    exr_file_t *file, int part_index,
    const exr_attr_chlist_t *channels )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(channels, EXR_ATTR_CHLIST );
    if ( rv == EXR_ERR_SUCCESS )
    {
        exr_attr_chlist_t *clist = attr->chlist;
        exr_attr_chlist_destroy( clist );
        if ( channels )
        {
            int numchans = channels->num_channels;
            rv = exr_attr_chlist_init( file, clist, numchans );
            if ( rv != EXR_ERR_SUCCESS )
                return rv;
            for ( int c = 0; c < numchans; ++c )
            {
                exr_attr_chlist_entry_t *cur = channels->entries + c;
                rv = exr_attr_chlist_add_with_length(
                    file, clist, cur->name.str, cur->name.length,
                    cur->pixel_type, cur->p_linear,
                    cur->x_sampling, cur->y_sampling );
                if ( rv != EXR_ERR_SUCCESS )
                    return rv;
            }
        }
    }
    return rv;
}

/**************************************/

exr_compression_t exr_get_compression(
    exr_file_t *file, int part_index )
{
    exr_compression_t retval = EXR_COMPRESSION_LAST_TYPE;
    REQ_ATTR_GET_IMPL(compression, uc);
    return retval;
}

/**************************************/

int exr_set_compression(
    exr_file_t *file, int part_index, exr_compression_t ctype )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(compression, EXR_ATTR_COMPRESSION );
    if ( rv == EXR_ERR_SUCCESS )
        attr->uc = (uint8_t)ctype;
    return rv;
}

/**************************************/

exr_attr_box2i_t exr_get_data_window(
    exr_file_t *file, int part_index )
{
    exr_attr_box2i_t ret = {0};
    exr_attr_box2i_t *retval = NULL;
    REQ_ATTR_GET_IMPL(dataWindow, box2i);
    if ( retval )
        ret = *(retval);
    return ret;
}

/**************************************/

int exr_set_data_window(
    exr_file_t *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(dataWindow, EXR_ATTR_BOX2I );
    if ( rv == EXR_ERR_SUCCESS )
    {
        attr->box2i->x_min = x_min;
        attr->box2i->y_min = y_min;
        attr->box2i->x_max = x_max;
        attr->box2i->y_max = y_max;
    }
    return rv;
}

/**************************************/

exr_attr_box2i_t exr_get_display_window(
    exr_file_t *file, int part_index )
{
    exr_attr_box2i_t ret = {0};
    exr_attr_box2i_t *retval = NULL;
    REQ_ATTR_GET_IMPL(displayWindow, box2i);
    if ( retval )
        ret = *(retval);
    return ret;
}

/**************************************/

int exr_set_display_window(
    exr_file_t *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(displayWindow, EXR_ATTR_BOX2I );
    if ( rv == EXR_ERR_SUCCESS )
    {
        attr->box2i->x_min = x_min;
        attr->box2i->y_min = y_min;
        attr->box2i->x_max = x_max;
        attr->box2i->y_max = y_max;
    }
    return rv;
}

/**************************************/

exr_lineorder_t exr_get_line_order(
    exr_file_t *file,
    int part_index )
{
    exr_lineorder_t retval = EXR_LINEORDER_LAST_TYPE;
    REQ_ATTR_GET_IMPL(lineOrder, uc);
    return retval;
}

/**************************************/

int exr_set_line_order(
    exr_file_t *file, int part_index, exr_lineorder_t lo )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(lineOrder, EXR_ATTR_LINEORDER );
    if ( rv == EXR_ERR_SUCCESS )
        attr->uc = (uint8_t)lo;
    return rv;
}

/**************************************/

float exr_get_pixel_aspect_ratio(
    exr_file_t *file, int part_index )
{
    float retval = 0.f;
    REQ_ATTR_GET_IMPL(pixelAspectRatio, f);
    return retval;
}

/**************************************/

int exr_set_pixel_aspect_ratio(
    exr_file_t *file, int part_index, float par )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(pixelAspectRatio, EXR_ATTR_FLOAT );
    if ( rv == EXR_ERR_SUCCESS )
        attr->f = par;
    return rv;
}

/**************************************/

exr_attr_v2f_t exr_get_screen_window_center(
    exr_file_t *file, int part_index )
{
    exr_attr_v2f_t *retval = NULL;
    exr_attr_v2f_t ret = {0};
    REQ_ATTR_GET_IMPL(screenWindowCenter, v2f);
    if ( retval )
        ret = *retval;
    return ret;
}

/**************************************/

int exr_set_screen_window_center(
    exr_file_t *file, int part_index,
    float x, float y )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(screenWindowCenter, EXR_ATTR_V2F );
    if ( rv == EXR_ERR_SUCCESS )
    {
        attr->v2f->x = x;
        attr->v2f->y = y;
    }
    return rv;
}

/**************************************/

float exr_get_screen_window_width(
    exr_file_t *file, int part_index )
{
    float retval = 0.f;
    REQ_ATTR_GET_IMPL(screenWindowWidth, f);
    return retval;
}

/**************************************/

int exr_set_screen_window_width(
    exr_file_t *file, int part_index, float ssw )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(screenWindowWidth, EXR_ATTR_FLOAT );
    if ( rv == EXR_ERR_SUCCESS )
        attr->f = ssw;
    return rv;
}

/**************************************/

uint32_t exr_get_tile_x_size(
    exr_file_t *file, int part_index )
{
    exr_attr_tiledesc_t *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? retval->x_size : 0;
}

/**************************************/

uint32_t exr_get_tile_y_size(
    exr_file_t *file, int part_index )
{
    exr_attr_tiledesc_t *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? retval->y_size : 0;
}

/**************************************/

exr_TILE_LEVEL_MODE_t exr_get_tile_level_mode(
    exr_file_t *file, int part_index )
{
    exr_attr_tiledesc_t *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? EXR_GET_TILE_LEVEL_MODE(*retval) : EXR_TILE_LAST_TYPE;
}

/**************************************/

exr_TILE_ROUND_MODE_t exr_get_tile_round_mode(
    exr_file_t *file, int part_index )
{
    exr_attr_tiledesc_t *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? EXR_GET_TILE_ROUND_MODE(*retval) : EXR_TILE_ROUND_LAST_TYPE;
}

/**************************************/

int exr_set_tile_descriptor(
    exr_file_t *file, int part_index,
    uint32_t x_size, uint32_t y_size,
    exr_TILE_LEVEL_MODE_t level_mode,
    exr_TILE_ROUND_MODE_t round_mode )
{
    exr_attribute_t *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(tiles, EXR_ATTR_TILEDESC );
    if ( rv == EXR_ERR_SUCCESS )
    {
        attr->tiledesc->x_size = x_size;
        attr->tiledesc->y_size = y_size;
        attr->tiledesc->level_and_round = EXR_PACK_TILE_LEVEL_ROUND(level_mode, round_mode);
    }
    return rv;
}

