/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_file.h"

#include <limits.h>
#include <math.h>

/**************************************/

static exr_result_t
validate_req_attr (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    if (!curpart->channels)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'channels' attribute not found");
    if (!curpart->compression)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'compression' attribute not found");
    if (!curpart->dataWindow)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'dataWindow' attribute not found");
    if (!curpart->displayWindow)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'displayWindow' attribute not found");
    if (!curpart->lineOrder)
        return f->print_error (
            f, EXR_ERR_MISSING_REQ_ATTR, "'lineOrder' attribute not found");
    if (!curpart->pixelAspectRatio)
        return f->print_error (
            f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'pixelAspectRatio' attribute not found");
    if (!curpart->screenWindowCenter)
        return f->print_error (
            f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'screenWindowCenter' attribute not found");
    if (!curpart->screenWindowWidth)
        return f->print_error (
            f,
            EXR_ERR_MISSING_REQ_ATTR,
            "'screenWindowWidth' attribute not found");

    if (f->is_multipart || f->has_nonimage_data)
    {
        if (f->is_multipart && !curpart->name)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'name' attribute for multipart file not found");
        if (!curpart->type)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'type' attribute for v2+ file not found");
        if (f->has_nonimage_data && !curpart->version)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'version' attribute for deep file not found");
        if (!curpart->chunkCount)
            return f->print_error (
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'chunkCount' attribute for multipart / deep file not found");
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
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

    w = (int64_t) dw.max.x - (int64_t) dw.min.x + 1;
    h = (int64_t) dw.max.y - (int64_t) dw.min.y + 1;

    if (dspw.min.x > dspw.max.x || dspw.min.y > dspw.max.y ||
        dspw.min.x <= -kLargeVal || dspw.min.y <= -kLargeVal ||
        dspw.max.x >= kLargeVal || dspw.max.y >= kLargeVal)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid display window (%d, %d - %d, %d)",
            dspw.min.x,
            dspw.min.y,
            dspw.max.x,
            dspw.max.y);

    if (dw.min.x > dw.max.x || dw.min.y > dw.max.y || dw.min.x <= -kLargeVal ||
        dw.min.y <= -kLargeVal || dw.max.x >= kLargeVal ||
        dw.max.y >= kLargeVal)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid data window (%d, %d - %d, %d)",
            dw.min.x,
            dw.min.y,
            dw.max.x,
            dw.max.y);

    if (maxw > 0 && maxw < w)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid width (%" PRId64 ") too large (max %d)",
            w,
            maxw);

    if (maxh > 0 && maxh < h)
        return f->print_error (
            f,
            EXR_ERR_INVALID_ATTR,
            "Invalid height (%" PRId64 ") too large (max %d)",
            h,
            maxh);

    if (maxw > 0 && maxh > 0)
    {
        int64_t maxNum = (int64_t) maxw * (int64_t) maxh;
        int64_t ccount = 0;
        if (curpart->chunkCount) ccount = (int64_t) curpart->chunk_count;
        if (ccount > maxNum)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid chunkCount (%" PRId64
                ") exceeds maximum area of %" PRId64 "",
                ccount,
                maxNum);
    }

    /* isnormal will return true when par is 0, which should also be disallowed */
    if (!isnormal (par) || par < 1e-6f || par > 1e+6f)
        return f->print_error (
            f, EXR_ERR_INVALID_ATTR, "Invalid pixel aspect ratio %g", (double)par);

    if (sww < 0.f)
        return f->print_error (
            f, EXR_ERR_INVALID_ATTR, "Invalid screen window width %g", (double)sww);

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
validate_channels (
    struct _internal_exr_context* f,
    struct _internal_exr_part*    curpart,
    const exr_attr_chlist_t*      channels)
{
    exr_attr_box2i_t dw;
    int64_t          w, h;

    if (!channels)
        return f->report_error (
            f,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing required channels attribute to validate against");
    if (!curpart->dataWindow)
        return f->report_error (
            f,
            EXR_ERR_NO_ATTR_BY_NAME,
            "request to validate channel list, but data window not set to validate against");

    dw = curpart->data_window;
    w  = dw.max.x - dw.min.x + 1;
    h  = dw.max.y - dw.min.y + 1;
    for (int c = 0; c < channels->num_channels; ++c)
    {
        int32_t xsamp = channels->entries[c].x_sampling;
        int32_t ysamp = channels->entries[c].y_sampling;
        if (xsamp < 1)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': x subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                xsamp);
        if (ysamp < 1)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': y subsampling factor is invalid (%d)",
                channels->entries[c].name.str,
                ysamp);
        if (dw.min.x % xsamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum x coordinate (%d) of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.min.x,
                xsamp);
        if (dw.min.y % ysamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': minimum y coordinate (%d) of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                dw.min.y,
                ysamp);
        if (w % xsamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': row width (%" PRId64
                ") of the data window is not a multiple of the x subsampling factor (%d)",
                channels->entries[c].name.str,
                w,
                xsamp);
        if (h % ysamp)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "channel '%s': column height (%" PRId64
                ") of the data window is not a multiple of the y subsampling factor (%d)",
                channels->entries[c].name.str,
                h,
                ysamp);
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
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
                f,
                EXR_ERR_INVALID_ATTR,
                "attribute 'type': Mismatch between file flags and type string '%s', believing file flags",
                curpart->type->string->str);

            if (f->mode == EXR_CONTEXT_WRITE) return EXR_ERR_INVALID_ATTR;

            rv = exr_attr_string_set_with_length (
                (exr_context_t) f, curpart->type->string, "tiledimage", 10);
            if (rv != EXR_ERR_SUCCESS)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "attribute 'type': Mismatch between file flags and type attribute, unable to fix");
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
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
                f,
                EXR_ERR_MISSING_REQ_ATTR,
                "'tiles' attribute for tiled file not found");

        desc = curpart->tiles->tiledesc;
        if (desc->x_size == 0 || desc->y_size == 0 ||
            desc->x_size > (uint32_t) INT_MAX ||
            desc->y_size > (uint32_t) INT_MAX)
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Invalid tile description size (%u x %u)",
                desc->x_size,
                desc->y_size);
        if (maxtilew > 0 && maxtilew < (int) (desc->x_size))
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->x_size,
                maxtilew);
        if (maxtileh > 0 && maxtileh < (int) (desc->y_size))
            return f->print_error (
                f,
                EXR_ERR_INVALID_ATTR,
                "Width of tile exceeds max size (%d vs max %d)",
                (int) desc->y_size,
                maxtileh);

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a tiled image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
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
                f, EXR_ERR_INVALID_ATTR, "Invalid compression for deep data");

        for (int c = 0; c < channels->num_channels; ++c)
        {
            if (channels->entries[c].x_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': x subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].x_sampling);
            if (channels->entries[c].y_sampling != 1)
                return f->print_error (
                    f,
                    EXR_ERR_INVALID_ATTR,
                    "channel '%s': y subsampling factor is not 1 (%d) for a deep image",
                    channels->entries[c].name.str,
                    channels->entries[c].y_sampling);
        }
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_exr_validate_read_part (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    exr_result_t rv;

    rv = validate_req_attr (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart, curpart->channels->chlist);
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

exr_result_t
internal_exr_validate_write_part (
    struct _internal_exr_context* f, struct _internal_exr_part* curpart)
{
    exr_result_t rv;

    rv = validate_req_attr (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_image_dimensions (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_channels (f, curpart, curpart->channels->chlist);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_part_type (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_tile_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    rv = validate_deep_data (f, curpart);
    if (rv != EXR_ERR_SUCCESS) return rv;

    return EXR_ERR_SUCCESS;
}
