/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_part.h"

#include "internal_attr.h"
#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_part_get_count (const exr_context_t ctxt, int* count)
{
    int cnt;
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);
    cnt = pctxt->num_parts;
    EXR_UNLOCK_WRITE (pctxt);

    if (!count) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *count = cnt;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_part_get_storage (
    const exr_context_t ctxt, int part_index, exr_storage_t* out)
{
    exr_storage_t smode;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    smode = part->storage_mode;
    EXR_UNLOCK_WRITE (pctxt);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = smode;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_part_add (
    exr_context_t ctxt,
    const char*   partname,
    exr_storage_t type,
    int*          new_index)
{
    uint8_t*     namestr;
    exr_result_t rv;
    int32_t      attrsz  = -1;
    const char*  typestr = NULL;

    struct _internal_exr_part* part = NULL;

    EXR_PROMOTE_LOCKED_CONTEXT_OR_ERROR (ctxt);

    if (pctxt->mode != EXR_CONTEXT_WRITE)
        return EXR_UNLOCK (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);

    rv = internal_exr_add_part (pctxt, &part, new_index);
    if (rv != EXR_ERR_SUCCESS) return EXR_UNLOCK (pctxt), rv;

    part->storage_mode = type;
    switch (type)
    {
        case EXR_STORAGE_SCANLINE:
            typestr = "scanlineimage";
            attrsz  = 13;
            break;
        case EXR_STORAGE_TILED:
            typestr = "tiledimage";
            attrsz  = 10;
            break;
        case EXR_STORAGE_DEEP_SCANLINE:
            typestr = "deepscanline";
            attrsz  = 12;
            break;
        case EXR_STORAGE_DEEP_TILED:
            typestr = "deeptile";
            attrsz  = 8;
            break;
        default:
            return EXR_UNLOCK (pctxt),
                   pctxt->print_error (
                       ctxt,
                       EXR_ERR_INVALID_ATTR,
                       "Invalid storage type %d for new part",
                       (int) type);
    }

    rv = exr_attr_list_add_static_name (
        ctxt,
        &(part->attributes),
        EXR_REQ_TYPE_STR,
        EXR_ATTR_STRING,
        0,
        NULL,
        &(part->type));

    if (rv != EXR_ERR_SUCCESS) return EXR_UNLOCK (pctxt), rv;

    rv = exr_attr_string_init_static_with_length (
        ctxt, part->type->string, typestr, attrsz);

    if (rv != EXR_ERR_SUCCESS) return EXR_UNLOCK (pctxt), rv;

    /* make sure we put in SOME sort of partname */
    if (!partname) partname = "";
    if (partname && partname[0] != '\0')
    {
        size_t pnamelen = strlen (partname);
        if (pnamelen >= INT32_MAX)
            return EXR_UNLOCK (pctxt),
                   pctxt->print_error (
                       ctxt,
                       EXR_ERR_INVALID_ATTR,
                       "Part name '%s': Invalid name length " PRIu64,
                       partname,
                       (uint64_t) pnamelen);

        rv = exr_attr_list_add_static_name (
            ctxt,
            &(part->attributes),
            EXR_REQ_NAME_STR,
            EXR_ATTR_STRING,
            0,
            NULL,
            &(part->name));

        if (rv == EXR_ERR_SUCCESS)
            rv = exr_attr_string_create_with_length (
                ctxt, part->name->string, partname, (int32_t) pnamelen);
    }

    if (rv == EXR_ERR_SUCCESS &&
        (type == EXR_STORAGE_DEEP_TILED || type == EXR_STORAGE_DEEP_SCANLINE))
    {
        rv = exr_attr_list_add_static_name (
            ctxt,
            &(part->attributes),
            EXR_REQ_VERSION_STR,
            EXR_ATTR_INT,
            0,
            NULL,
            &(part->version));
        if (rv == EXR_ERR_SUCCESS) part->version->i = 1;
        pctxt->has_nonimage_data = 1;
    }

    if (rv == EXR_ERR_SUCCESS)
    {
        if (pctxt->num_parts > 1) pctxt->is_multipart = 1;

        if (!pctxt->has_nonimage_data && pctxt->num_parts == 1 &&
            type == EXR_STORAGE_TILED)
            pctxt->is_singlepart_tiled = 1;
        else
            pctxt->is_singlepart_tiled = 0;
    }

    EXR_UNLOCK (pctxt);
    return rv;
}

/**************************************/

exr_result_t
exr_part_get_tile_levels (
    const exr_context_t ctxt, int part_index, int* levelsx, int* levelsy)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_RETURN_WRITE (pctxt),
                   pctxt->print_error (
                       ctxt,
                       EXR_ERR_BAD_CHUNK_DATA,
                       "Request for tile, but no tile data exists");
        }

        if (levelsx) *levelsx = part->num_tile_levels_x;
        if (levelsy) *levelsy = part->num_tile_levels_y;
        return EXR_RETURN_WRITE (pctxt), EXR_ERR_SUCCESS;
    }

    return EXR_RETURN_WRITE (pctxt),
           pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
}

/**************************************/

exr_result_t
exr_part_get_tile_sizes (
    const exr_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            tilew,
    int32_t*            tileh)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        const exr_attr_tiledesc_t* tiledesc;

        if (!part->tiles || part->num_tile_levels_x <= 0 ||
            part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
            !part->tile_level_tile_count_y)
        {
            return EXR_RETURN_WRITE (pctxt),
                   pctxt->print_error (
                       ctxt,
                       EXR_ERR_BAD_CHUNK_DATA,
                       "Request for tile, but no tile data exists");
        }

        if (levelx >= part->num_tile_levels_x ||
            levely >= part->num_tile_levels_y)
            return EXR_RETURN_WRITE (pctxt),
                   pctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE);

        tiledesc = part->tiles->tiledesc;
        if (tilew)
        {
            *tilew = part->tile_level_tile_size_x[levelx];
            if (tiledesc->x_size < (uint32_t) *tilew) *tilew = tiledesc->x_size;
        }
        if (tileh)
        {
            *tileh = part->tile_level_tile_size_y[levely];
            if (tiledesc->y_size < (uint32_t) *tileh) *tileh = tiledesc->y_size;
        }
        return EXR_RETURN_WRITE (pctxt), EXR_ERR_SUCCESS;
    }

    return EXR_RETURN_WRITE (pctxt),
           pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
}

/**************************************/

exr_result_t
exr_part_get_chunk_count (
    const exr_context_t ctxt, int part_index, int32_t* out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out)
        return EXR_RETURN_WRITE (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->dataWindow)
    {
        if (part->storage_mode == EXR_STORAGE_TILED ||
            part->storage_mode == EXR_STORAGE_DEEP_TILED)
        {
            if (part->tiles)
            {
                *out = part->chunk_count;
                return EXR_RETURN_WRITE (pctxt), EXR_ERR_SUCCESS;
            }
            return EXR_RETURN_WRITE (pctxt),
                   pctxt->report_error (
                       ctxt,
                       EXR_ERR_BAD_CHUNK_DATA,
                       "Missing tile chunk information");
        }
        else if (
            part->storage_mode == EXR_STORAGE_SCANLINE ||
            part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
        {
            if (part->compression)
            {
                *out = part->chunk_count;
                return EXR_RETURN_WRITE (pctxt), EXR_ERR_SUCCESS;
            }
            return EXR_RETURN_WRITE (pctxt),
                   pctxt->report_error (
                       ctxt,
                       EXR_ERR_BAD_CHUNK_DATA,
                       "Missing scanline chunk compression information");
        }
    }

    return EXR_RETURN_WRITE (pctxt),
           pctxt->report_error (
               ctxt,
               EXR_ERR_BAD_CHUNK_DATA,
               "Missing data window for chunk information");
}

/**************************************/

exr_result_t
exr_part_get_scanlines_per_chunk (
    const exr_context_t ctxt, int part_index, int32_t* out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out)
        return EXR_RETURN_WRITE (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        *out = part->lines_per_chunk;
        return EXR_RETURN_WRITE (pctxt), EXR_ERR_SUCCESS;
    }
    return EXR_RETURN_WRITE (pctxt),
           pctxt->report_error (
               ctxt,
               EXR_ERR_BAD_CHUNK_DATA,
               "Request for scanlines per chunk, but inspecting a scanline-stored part");
}

/**************************************/

exr_result_t
exr_part_get_chunk_unpacked_size (
    const exr_context_t ctxt, int part_index, uint64_t* out)
{
    uint64_t sz;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    sz = part->unpacked_size_per_chunk;
    EXR_UNLOCK_WRITE (pctxt);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = sz;
    return EXR_ERR_SUCCESS;
}
