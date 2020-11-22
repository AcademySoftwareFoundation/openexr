/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_req_attr.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"

/**************************************/

#define REQ_ATTR_GET_IMPL(name, entry) \
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);                 \
    if ( f && part_index >= 0 && part_index < f->num_parts )    \
    {                                                           \
        EXR_TYPE(PRIV_PART) *part = f->parts[part_index];       \
        if ( part->name )                                       \
            retval = part->name->entry;                         \
    }

#define REQ_ATTR_FIND_CREATE(name, type)                        \
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);                 \
    if ( f && part_index >= 0 && part_index < f->num_parts )    \
    {                                                           \
        EXR_TYPE(PRIV_PART) *part = f->parts[part_index];       \
        if ( part->name )                                       \
        {                                                       \
            attr = part->name;                                  \
            rv = EXR_DEF(ERR_SUCCESS);                          \
        }                                                       \
        else                                                    \
        {                                                       \
            rv = EXR_FUN(attr_list_add)(                        \
                file, &(part->attributes), #name,               \
                type, 0, NULL, &(part->name) );                 \
            if ( rv == EXR_DEF(ERR_SUCCESS) )                   \
                attr = part->name;                              \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        rv = f->print_error(                                    \
            file, EXR_DEF(ERR_INVALID_ARGUMENT),                \
            "Missing file or invalid part number (%d)",         \
            part_index );                                       \
    }

/**************************************/

const EXR_TYPE(attr_chlist) *EXR_FUN(get_channels)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_chlist) *retval = NULL;
    REQ_ATTR_GET_IMPL(channels, chlist);
    return retval;
}

/**************************************/

int EXR_FUN(add_channels)(
    EXR_TYPE(FILE) *file, int part_index,
    const char *name,
    EXR_TYPE(PIXEL_TYPE) ptype,
    uint8_t islinear,
    int32_t xsamp, int32_t ysamp )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;
    REQ_ATTR_FIND_CREATE(channels, EXR_DEF(ATTR_CHLIST) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        rv = EXR_FUN(attr_chlist_add)(
            file, attr->chlist, name, ptype, islinear, xsamp, ysamp );
    }
    return rv;
}

/**************************************/

int EXR_FUN(set_channels)(
    EXR_TYPE(FILE) *file, int part_index,
    const EXR_TYPE(attr_chlist) *channels )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(channels, EXR_DEF(ATTR_CHLIST) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        EXR_TYPE(attr_chlist) *clist = attr->chlist;
        EXR_FUN(attr_chlist_destroy)( clist );
        if ( channels )
        {
            int numchans = channels->num_channels;
            rv = EXR_FUN(attr_chlist_init)( file, clist, numchans );
            if ( rv != EXR_DEF(ERR_SUCCESS) )
                return rv;
            for ( int c = 0; c < numchans; ++c )
            {
                EXR_TYPE(attr_chlist_entry) *cur = channels->entries + c;
                rv = EXR_FUN(attr_chlist_add_with_length)(
                    file, clist, cur->name.str, cur->name.length,
                    cur->pixel_type, cur->p_linear,
                    cur->x_sampling, cur->y_sampling );
                if ( rv != EXR_DEF(ERR_SUCCESS) )
                    return rv;
            }
        }
    }
    return rv;
}

/**************************************/

EXR_TYPE(COMPRESSION_TYPE) EXR_FUN(get_compression)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(COMPRESSION_TYPE) retval = EXR_DEF(COMPRESSION_LAST_TYPE);
    REQ_ATTR_GET_IMPL(compression, uc);
    return retval;
}

/**************************************/

int EXR_FUN(set_compression)(
    EXR_TYPE(FILE) *file, int part_index, EXR_TYPE(COMPRESSION_TYPE) ctype )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(compression, EXR_DEF(ATTR_COMPRESSION) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
        attr->uc = (uint8_t)ctype;
    return rv;
}

/**************************************/

EXR_TYPE(attr_box2i) EXR_FUN(get_data_window)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_box2i) ret = {0};
    EXR_TYPE(attr_box2i) *retval = NULL;
    REQ_ATTR_GET_IMPL(dataWindow, box2i);
    if ( retval )
        ret = *(retval);
    return ret;
}

/**************************************/

int EXR_FUN(set_data_window)(
    EXR_TYPE(FILE) *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(dataWindow, EXR_DEF(ATTR_BOX2I) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        attr->box2i->x_min = x_min;
        attr->box2i->y_min = y_min;
        attr->box2i->x_max = x_max;
        attr->box2i->y_max = y_max;
    }
    return rv;
}

/**************************************/

EXR_TYPE(attr_box2i) EXR_FUN(get_display_window)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_box2i) ret = {0};
    EXR_TYPE(attr_box2i) *retval = NULL;
    REQ_ATTR_GET_IMPL(displayWindow, box2i);
    if ( retval )
        ret = *(retval);
    return ret;
}

/**************************************/

int EXR_FUN(set_display_window)(
    EXR_TYPE(FILE) *file, int part_index,
    int32_t x_min, int32_t y_min, int32_t x_max, int32_t y_max )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(displayWindow, EXR_DEF(ATTR_BOX2I) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        attr->box2i->x_min = x_min;
        attr->box2i->y_min = y_min;
        attr->box2i->x_max = x_max;
        attr->box2i->y_max = y_max;
    }
    return rv;
}

/**************************************/

EXR_TYPE(LINEORDER_TYPE) EXR_FUN(get_line_order)(
    EXR_TYPE(FILE) *file,
    int part_index )
{
    EXR_TYPE(LINEORDER_TYPE) retval = EXR_DEF(LINEORDER_LAST_TYPE);
    REQ_ATTR_GET_IMPL(lineOrder, uc);
    return retval;
}

/**************************************/

int EXR_FUN(set_line_order)(
    EXR_TYPE(FILE) *file, int part_index, EXR_TYPE(LINEORDER_TYPE) lo )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(lineOrder, EXR_DEF(ATTR_LINEORDER) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
        attr->uc = (uint8_t)lo;
    return rv;
}

/**************************************/

float EXR_FUN(get_pixel_aspect_ratio)(
    EXR_TYPE(FILE) *file, int part_index )
{
    float retval = 0.f;
    REQ_ATTR_GET_IMPL(pixelAspectRatio, f);
    return retval;
}

/**************************************/

int EXR_FUN(set_pixel_aspect_ratio)(
    EXR_TYPE(FILE) *file, int part_index, float par )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(pixelAspectRatio, EXR_DEF(ATTR_FLOAT) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
        attr->f = par;
    return rv;
}

/**************************************/

EXR_TYPE(attr_v2f) EXR_FUN(get_screen_window_center)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_v2f) *retval = NULL;
    EXR_TYPE(attr_v2f) ret = {0};
    REQ_ATTR_GET_IMPL(screenWindowCenter, v2f);
    if ( retval )
        ret = *retval;
    return ret;
}

/**************************************/

int EXR_FUN(set_screen_window_center)(
    EXR_TYPE(FILE) *file, int part_index,
    float x, float y )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(screenWindowCenter, EXR_DEF(ATTR_V2F) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        attr->v2f->x = x;
        attr->v2f->y = y;
    }
    return rv;
}

/**************************************/

float EXR_FUN(get_screen_window_width)(
    EXR_TYPE(FILE) *file, int part_index )
{
    float retval = 0.f;
    REQ_ATTR_GET_IMPL(screenWindowWidth, f);
    return retval;
}

/**************************************/

int EXR_FUN(set_screen_window_width)(
    EXR_TYPE(FILE) *file, int part_index, float ssw )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(screenWindowWidth, EXR_DEF(ATTR_FLOAT) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
        attr->f = ssw;
    return rv;
}

/**************************************/

uint32_t EXR_FUN(get_tile_x_size)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_tiledesc) *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? retval->x_size : 0;
}

/**************************************/

uint32_t EXR_FUN(get_tile_y_size)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_tiledesc) *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? retval->y_size : 0;
}

/**************************************/

EXR_TYPE(TILE_LEVEL_MODE) EXR_FUN(get_tile_level_mode)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_tiledesc) *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? EXR_GET_TILE_LEVEL_MODE(*retval) : EXR_DEF(TILE_LAST_TYPE);
}

/**************************************/

EXR_TYPE(TILE_ROUND_MODE) EXR_FUN(get_tile_round_mode)(
    EXR_TYPE(FILE) *file, int part_index )
{
    EXR_TYPE(attr_tiledesc) *retval = NULL;
    REQ_ATTR_GET_IMPL(tiles, tiledesc);
    return retval ? EXR_GET_TILE_ROUND_MODE(*retval) : EXR_DEF(TILE_ROUND_LAST_TYPE);
}

/**************************************/

int EXR_FUN(set_tile_descriptor)(
    EXR_TYPE(FILE) *file, int part_index,
    uint32_t x_size, uint32_t y_size,
    EXR_TYPE(TILE_LEVEL_MODE) level_mode,
    EXR_TYPE(TILE_ROUND_MODE) round_mode )
{
    EXR_TYPE(attribute) *attr = NULL;
    int rv;

    /* TODO: validate here or later? */
    REQ_ATTR_FIND_CREATE(tiles, EXR_DEF(ATTR_TILEDESC) );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
    {
        attr->tiledesc->x_size = x_size;
        attr->tiledesc->y_size = y_size;
        attr->tiledesc->level_and_round = EXR_PACK_TILE_LEVEL_ROUND(level_mode, round_mode);
    }
    return rv;
}

