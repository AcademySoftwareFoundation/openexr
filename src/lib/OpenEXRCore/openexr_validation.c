/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_priv_file.h"

#include <limits.h>
#include <math.h>

/**************************************/

static int
validate_req_attr (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    if (!curpart->channels)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'channels' attribute not found");
    if (!curpart->compression)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'compression' attribute not found");
    if (!curpart->dataWindow)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'dataWindow' attribute not found");
    if (!curpart->displayWindow)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'displayWindow' attribute not found");
    if (!curpart->lineOrder)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'lineOrder' attribute not found");
    if (!curpart->pixelAspectRatio)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'pixelAspectRatio' attribute not found");
    if (!curpart->screenWindowCenter)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'screenWindowCenter' attribute not found");
    if (!curpart->screenWindowWidth)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'screenWindowWidth' attribute not found");

    if (f->is_multipart || f->has_nonimage_data)
    {
        if (!curpart->name)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'name' attribute for multipart / deep file not found");
        if (!curpart->type)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'type' attribute for multipart / deep file not found");
        if (f->has_nonimage_data && !curpart->version)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'version' attribute for deep file not found");
        if (!curpart->chunkCount)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'chunkCount' attribute for multipart / deep file not found");
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
validate_image_dimensions (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    // sanity check the various parts...
    const int64_t          kLargeVal = (int64_t) (INT32_MAX / 2);
    const exr_attr_box2i_t dw        = curpart->data_window;
    const exr_attr_box2i_t dspw      = curpart->display_window;
    int64_t                w, h;
    float                  par, sww;
    int                    maxw = f->max_image_w;
    int                    maxh = f->max_image_h;

    par = curpart->pixelAspectRatio->f;
    sww = curpart->screenWindowWidth->f;

    w = (int64_t) dw.x_max - (int64_t) dw.x_min + 1;
    h = (int64_t) dw.y_max - (int64_t) dw.y_min + 1;

    if (dspw.x_min > dspw.x_max || dspw.y_min > dspw.y_max ||
        dspw.x_min <= -kLargeVal || dspw.y_min <= -kLargeVal ||
        dspw.x_max >= kLargeVal || dspw.y_max >= kLargeVal)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid display window (%d, %d - %d, %d)",
            dspw.x_min,
            dspw.y_min,
            dspw.x_max,
            dspw.y_max);

    if (dw.x_min > dw.x_max || dw.y_min > dw.y_max || dw.x_min <= -kLargeVal ||
        dw.y_min <= -kLargeVal || dw.x_max >= kLargeVal ||
        dw.y_max >= kLargeVal)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid data window (%d, %d - %d, %d)",
            dw.x_min,
            dw.y_min,
            dw.x_max,
            dw.y_max);

    if (maxw > 0 && maxw < w)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid width (%ld) too large (max %d)",
            w,
            maxw);

    if (maxh > 0 && maxh < h)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid height (%ld) too large (max %d)",
            h,
            maxh);

    if (maxw > 0 && maxh > 0)
    {
        int64_t maxNum = (int64_t) maxw * (int64_t) maxh;
        int64_t ccount = 0;
        if (curpart->chunkCount) ccount = (int64_t) curpart->chunk_count;
        if (ccount > maxNum)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "Invalid chunkCount (%ld) exceeds maximum area of %ld",
                ccount,
                maxNum);
    }

    /* isnormal will return true when par is 0, which should also be disallowed */
    if (!isnormal (par) || par < 1e-6f || par > 1e+6f)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid pixel aspect ratio %g",
            par);

    if (sww < 0.f)
        return f->print_error (
            (const exr_context_t) f,
            EXR_ERR_INVALID_ATTR,
            "Invalid screen window width %g",
            sww);

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
validate_channels (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    const exr_attr_chlist_t* channels = curpart->channels->chlist;
    const exr_attr_box2i_t   dw       = curpart->data_window;
    int64_t                  w, h;

    w = dw.x_max - dw.x_min + 1;
    h = dw.y_max - dw.y_min + 1;

    for (int c = 0; c < channels->num_channels; ++c)
    {
        int32_t xsamp = channels->entries[c].x_sampling;
        int32_t ysamp = channels->entries[c].y_sampling;
        if (xsamp < 1)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': x subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                xsamp);
        if (ysamp < 1)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': y subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                ysamp);
        if (dw.x_min % xsamp)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum x coordinate (%d) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.x_min,
                xsamp);
        if (dw.y_min % ysamp)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum y coordinate (%d) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.y_min,
                ysamp);
        if (w % xsamp)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': row width (%ld) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                w,
                xsamp);
        if (h % ysamp)
            return f->print_error (
                (const exr_context_t)f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': column height (%ld) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                h,
                ysamp);
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
validate_part_type (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    // TODO: there are probably more tests to add here...
    if (curpart->type)
    {
        int rv;

        // see if the type overwrote the storage mode
        if (f->is_singlepart_tiled &&
            curpart->storage_mode != EXR_STORAGE_TILED)
        {
            // mis-match between type attr and file flag. c++ believed the
            // flag first and foremost
            curpart->storage_mode = EXR_STORAGE_TILED;

            // TODO: define how strict we should be
            //exr_attr_list_remove( f, &(curpart->attributes), curpart->type );
            //curpart->type = NULL;
            f->print_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "attribute 'type': Mismatch between file flags and type string '%s', believing file flags",
                curpart->type->string->str);

            if (f->mode == EXR_CONTEXT_WRITE) return EXR_ERR_INVALID_ATTR;

            rv = exr_attr_string_set_with_length (
                f, curpart->type->string, "tiledimage", 10);
            if (rv != EXR_ERR_SUCCESS)
                return f->print_error (
                    (const exr_context_t) f,
                    EXR_ERR_INVALID_ATTR,
                    "attribute 'type': Mismatch between file flags and type attribute, unable to fix");
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
validate_tile_data (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    if (curpart->storage_mode == EXR_STORAGE_TILED ||
        curpart->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_tiledesc_t* desc;
        const int                  maxtilew = f->max_tile_w;
        const int                  maxtileh = f->max_tile_h;
        const exr_attr_chlist_t*   channels = curpart->channels->chlist;

        if (!curpart->tiles)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'tiles' attribute for tiled file not found");

        desc = curpart->tiles->tiledesc;
        if (desc->x_size == 0 || desc->y_size == 0 ||
            desc->x_size > (uint32_t) INT_MAX ||
            desc->y_size > (uint32_t) INT_MAX)
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "Invalid tile description size (%u x %u)",
                desc->x_size,
                desc->y_size);
        if (maxtilew > 0 && maxtilew < (int) (desc->x_size))
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->x_size,
                maxtilew);
        if (maxtileh > 0 && maxtileh < (int) (desc->y_size))
            return f->print_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->y_size,
                maxtileh);

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    (const exr_context_t) f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    (const exr_context_t) f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
validate_deep_data (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    if (curpart->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        curpart->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_chlist_t* channels = curpart->channels->chlist;

        // none, rle, zips
        if (curpart->comp_type != EXR_COMPRESSION_NONE &&
            curpart->comp_type != EXR_COMPRESSION_RLE &&
            curpart->comp_type != EXR_COMPRESSION_ZIPS)
            return f->report_error (
                (const exr_context_t) f,
                EXR_ERR_INVALID_ATTR,
                "Invalid compression for deep data");

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    (const exr_context_t) f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    (const exr_context_t) f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

int
internal_exr_validate_read_part (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    int rv;

    rv = validate_req_attr (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_part_type (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_tile_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_deep_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return EXR_ERR_SUCCESS;
}

/**************************************/

int
internal_exr_validate_write_part (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    int rv;

    rv = validate_req_attr (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_part_type (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_tile_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_deep_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return EXR_ERR_SUCCESS;
}
