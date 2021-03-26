/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_part.h"

#include "openexr_priv_constants.h"
#include "openexr_priv_structs.h"

/**************************************/

exr_result_t
exr_get_part_count (const exr_context_t ctxt, int* count)
{
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);

    if (!count) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *count = pctxt->num_parts;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_part_name (const exr_context_t ctxt, int part_index, const char** out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = part->name->string->str;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_part_storage (const exr_context_t ctxt, int part_index, exr_storage_t* out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    *out = part->storage_mode;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_get_tile_levels (
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
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for tile, but no tile data exists");
        }

        if (levelsx) *levelsx = part->num_tile_levels_x;
        if (levelsy) *levelsy = part->num_tile_levels_y;
        return EXR_ERR_SUCCESS;
    }

    return pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
}

/**************************************/

exr_result_t
exr_get_tile_sizes (
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
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for tile, but no tile data exists");
        }

        if (levelx >= part->num_tile_levels_x ||
            levely >= part->num_tile_levels_y)
            return pctxt->standard_error (ctxt, EXR_ERR_ARGUMENT_OUT_OF_RANGE);

        tiledesc = part->tiles->tiledesc;
        if (tilew)
        {
            *tilew = part->tile_level_tile_size_x[levelx];
            if (tiledesc->x_size < *tilew) *tilew = tiledesc->x_size;
        }
        if (tileh)
        {
            *tileh = part->tile_level_tile_size_y[levely];
            if (tiledesc->y_size < *tileh) *tileh = tiledesc->y_size;
        }
        return EXR_ERR_SUCCESS;
    }

    return pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
}

/**************************************/

exr_result_t
exr_get_chunk_count (const exr_context_t ctxt, int part_index, int32_t* out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->dataWindow)
    {
        if (part->storage_mode == EXR_STORAGE_TILED ||
            part->storage_mode == EXR_STORAGE_DEEP_TILED)
        {
            if (part->tiles)
            {
                *out = part->chunk_count;
                return EXR_ERR_SUCCESS;
            }
            return pctxt->report_error (
                ctxt, EXR_ERR_BAD_CHUNK_DATA, "Missing tile chunk information");
        }
        else if (
            part->storage_mode == EXR_STORAGE_SCANLINE ||
            part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
        {
            if (part->compression)
            {
                *out = part->chunk_count;
                return EXR_ERR_SUCCESS;
            }
            return pctxt->report_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Missing scanline chunk compression information");
        }
    }

    return pctxt->report_error (
        ctxt,
        EXR_ERR_BAD_CHUNK_DATA,
        "Missing data window for chunk information");
}

/**************************************/

exr_result_t
exr_get_scanlines_per_chunk (
    const exr_context_t ctxt, int part_index, int32_t* out)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!out) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        *out = part->lines_per_chunk;
        return EXR_ERR_SUCCESS;
    }
    return pctxt->report_error (
        ctxt,
        EXR_ERR_BAD_CHUNK_DATA,
        "Request for scanlines per chunk, but inspecting a scanline-stored part");
}
