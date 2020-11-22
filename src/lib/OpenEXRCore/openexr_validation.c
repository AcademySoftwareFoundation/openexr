/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_priv_file.h"

#include <math.h>
#include <limits.h>

/**************************************/

static int validate_req_attr( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);

    if ( ! curpart->channels )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'channels' attribute not found" );
    if ( ! curpart->compression )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'compression' attribute not found" );
    if ( ! curpart->dataWindow )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'dataWindow' attribute not found" );
    if ( ! curpart->displayWindow )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'displayWindow' attribute not found" );
    if ( ! curpart->lineOrder )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'lineOrder' attribute not found" );
    if ( ! curpart->pixelAspectRatio )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'pixelAspectRatio' attribute not found" );
    if ( ! curpart->screenWindowCenter )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'screenWindowCenter' attribute not found" );
    if ( ! curpart->screenWindowWidth )
        return file->print_error(
            f, EXR_DEF(ERR_MISSING_REQ_ATTR),
            "'screenWindowWidth' attribute not found" );

    if ( file->is_multipart || file->has_nonimage_data )
    {
        if ( ! curpart->name )
            return file->print_error(
                f, EXR_DEF(ERR_MISSING_REQ_ATTR),
                "'name' attribute for multipart / deep file not found" );
        if ( ! curpart->type )
            return file->print_error(
                f, EXR_DEF(ERR_MISSING_REQ_ATTR),
                "'type' attribute for multipart / deep file not found" );
        if ( file->has_nonimage_data && ! curpart->version )
            return file->print_error(
                f, EXR_DEF(ERR_MISSING_REQ_ATTR),
                "'version' attribute for deep file not found" );
        if ( ! curpart->chunkCount )
            return file->print_error(
                f, EXR_DEF(ERR_MISSING_REQ_ATTR),
                "'chunkCount' attribute for multipart / deep file not found" );
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_image_dimensions( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    // sanity check the various parts...
    const int64_t kLargeVal = (int64_t)(INT32_MAX / 2);
    const EXR_TYPE(attr_box2i) dw = curpart->data_window;
    const EXR_TYPE(attr_box2i) dspw = curpart->display_window;
    int64_t w, h;
    float par, sww;
    int rv;
    int maxw = EXR_FUN(get_maximum_image_width)();
    int maxh = EXR_FUN(get_maximum_image_height)();

    par = curpart->pixelAspectRatio->f;
    sww = curpart->screenWindowWidth->f;

    w = (int64_t)dw.x_max - (int64_t)dw.x_min + 1;
    h = (int64_t)dw.y_max - (int64_t)dw.y_min + 1;
    
    if ( dspw.x_min > dspw.x_max || dspw.y_min > dspw.y_max ||
         dspw.x_min <= -kLargeVal || dspw.y_min <= -kLargeVal ||
         dspw.x_max >= kLargeVal || dspw.y_max >= kLargeVal )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Invalid display window (%d, %d - %d, %d)",
            dspw.x_min, dspw.y_min, dspw.x_max, dspw.y_max );

    if ( dw.x_min > dw.x_max || dw.y_min > dw.y_max ||
         dw.x_min <= -kLargeVal || dw.y_min <= -kLargeVal ||
         dw.x_max >= kLargeVal || dw.y_max >= kLargeVal )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Invalid data window (%d, %d - %d, %d)",
            dw.x_min, dw.y_min, dw.x_max, dw.y_max );

    if ( maxw > 0 && maxw < w )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR), "Invalid width (%ld) too large (max %d)", w, maxw );
    
    if ( maxh > 0 && maxh < h )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR), "Invalid height (%ld) too large (max %d)", h, maxh );

    if ( maxw > 0 && maxh > 0 )
    {
        int64_t maxNum = (int64_t)maxw * (int64_t)maxh;
        int64_t ccount = 0;
        if ( curpart->chunkCount )
            ccount = (int64_t)curpart->chunk_count;
        if ( ccount > maxNum )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "Invalid chunkCount (%ld) exceeds maximum area of %ld",
                ccount, maxNum );
    }
            
    if ( ! isnormal( par ) || par < 1e-6f || par > 1e+6f )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR), "Invalid pixel aspect ratio %g", par );

    if ( sww < 0.f )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR), "Invalid screen window width %g", sww );

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_channels( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    const EXR_TYPE(attr_chlist) *channels = curpart->channels->chlist;
    const EXR_TYPE(attr_box2i) dw = curpart->data_window;
    int64_t w, h;

    w = dw.x_max - dw.x_min + 1;
    h = dw.y_max - dw.y_min + 1;

    for ( int c = 0; c < channels->num_channels; ++c )
    {
        int32_t xsamp = channels->entries[c].x_sampling;
        int32_t ysamp = channels->entries[c].y_sampling;
        if ( xsamp < 1 )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': x subsampling factor is invalid (%d)",
                channels->entries[c].name.str, xsamp );
        if ( ysamp < 1 )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': y subsampling factor is invalid (%d)",
                channels->entries[c].name.str, ysamp );
        if ( dw.x_min % xsamp )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': minimum x coordinate (%d) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str, dw.x_min, xsamp );
        if ( dw.y_min % ysamp )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': minimum y coordinate (%d) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str, dw.y_min, ysamp );
        if ( w % xsamp )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': row width (%ld) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str, w, xsamp );
        if ( h % ysamp )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "channel '%s': column height (%ld) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str, h, ysamp );
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_part_type( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart, int iswrite )
{
    // TODO: there are probably more tests to add here...
    if ( curpart->type )
    {
        int rv;
        EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);

        // see if the type overwrote the storage mode
        if ( file->is_singlepart_tiled && curpart->storage_mode != EXR_DEF(STORAGE_TILED) )
        {
            // mis-match between type attr and file flag. c++ believed the
            // flag first and foremost
            curpart->storage_mode = EXR_DEF(STORAGE_TILED);

            // TODO: define how strict we should be
            //EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->type );
            //curpart->type = NULL;
            file->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "attribute 'type': Mismatch between file flags and type string '%s', believing file flags", curpart->type->string->str );

            if ( iswrite )
                return EXR_DEF(ERR_INVALID_ATTR);

            rv = EXR_FUN(attr_string_set_with_length)( f, curpart->type->string, "tiledimage", 10 );
            if ( rv != EXR_DEF(ERR_SUCCESS) )
                return file->print_error(
                    f, EXR_DEF(ERR_INVALID_ATTR),
                    "attribute 'type': Mismatch between file flags and type attribute, unable to fix" );
        }
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_tile_data( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    if ( curpart->storage_mode == EXR_DEF(STORAGE_TILED) ||
         curpart->storage_mode == EXR_DEF(STORAGE_DEEP_TILED) )
    {
        const EXR_TYPE(attr_tiledesc) *desc;
        const int maxtilew = EXR_FUN(get_maximum_tile_width)();
        const int maxtileh = EXR_FUN(get_maximum_tile_height)();
        const EXR_TYPE(attr_chlist) *channels = curpart->channels->chlist;

        if ( ! curpart->tiles )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_MISSING_REQ_ATTR),
                "'tiles' attribute for tiled file not found" );

        desc = curpart->tiles->tiledesc;
        if ( desc->x_size == 0 || desc->y_size == 0 ||
             desc->x_size > (uint32_t)INT_MAX || desc->y_size > (uint32_t)INT_MAX )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "Invalid tile description size (%u x %u)",
                desc->x_size, desc->y_size );
        if ( maxtilew > 0 && maxtilew < (int)(desc->x_size) )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "Width of tile exceeds max size (%d vs max %d)",
                (int)desc->x_size, maxtilew );
        if ( maxtileh > 0 && maxtileh < (int)(desc->y_size) )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "Width of tile exceeds max size (%d vs max %d)",
                (int)desc->y_size, maxtileh );

        for ( int c = 0; c < channels->num_channels; ++c )
        {
            if ( channels->entries[c].x_sampling != 1 )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ATTR),
                    "channel '%s': x subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str, channels->entries[c].x_sampling );
            if ( channels->entries[c].y_sampling != 1 )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ATTR),
                    "channel '%s': y subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str, channels->entries[c].y_sampling );
        }
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int validate_deep_data( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    if ( curpart->storage_mode == EXR_DEF(STORAGE_DEEP_SCANLINE) ||
         curpart->storage_mode == EXR_DEF(STORAGE_DEEP_TILED) )
    {
        const EXR_TYPE(attr_chlist) *channels = curpart->channels->chlist;

        // none, rle, zips
        if ( curpart->comp_type != EXR_DEF(COMPRESSION_NONE) &&
             curpart->comp_type != EXR_DEF(COMPRESSION_RLE) &&
             curpart->comp_type != EXR_DEF(COMPRESSION_ZIPS) )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ATTR), "Invalid compression for deep data" );

        for ( int c = 0; c < channels->num_channels; ++c )
        {
            if ( channels->entries[c].x_sampling != 1 )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ATTR),
                    "channel '%s': x subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str, channels->entries[c].x_sampling );
            if ( channels->entries[c].y_sampling != 1 )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ATTR),
                    "channel '%s': y subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str, channels->entries[c].y_sampling );
        }
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int priv_validate_read_part( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    int rv;

    rv = validate_req_attr( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_image_dimensions( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_channels( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_part_type( f, curpart, 0 );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_tile_data( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_deep_data( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int priv_validate_write_part( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    int rv;

    rv = validate_req_attr( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_image_dimensions( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_channels( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_part_type( f, curpart, 1 );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_tile_data( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    rv = validate_deep_data( f, curpart );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
        return rv;

    return EXR_DEF(ERR_SUCCESS);
}
