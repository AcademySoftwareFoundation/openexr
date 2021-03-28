/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_read.h"

#include "internal_structs.h"
#include "internal_xdr.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/**************************************/

static exr_result_t
initialize_part_read (
    struct _internal_exr_context* ctxt,
    struct _internal_exr_part*    part,
    exr_decode_chunk_info_t*      cinfo,
    uint64_t**                    chunktable)
{
    //exr_decode_chunk_info_t nil = {0};
    uint64_t*                  ctable = NULL;
    int                        chans;
    exr_attr_chlist_t*         chanlist;
    exr_channel_decode_info_t* chandecodes;

    if (!cinfo)
        return ctxt->report_error (
            (const exr_context_t) ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing chunk info pointer to compute scanline chunk");

    //*cinfo = nil;
    ctable = (uint64_t*) atomic_load (&(part->chunk_table));
    if (ctable == NULL)
    {
        uint64_t  chunkoff   = part->chunk_table_offset;
        uint64_t  chunkbytes = sizeof (uint64_t) * (uint64_t) part->chunk_count;
        int64_t   nread      = 0;
        uintptr_t eptr = 0, nptr = 0;

        exr_result_t rv;

        if (part->chunk_count <= 0)
            return ctxt->report_error (
                (const exr_context_t) ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid file with no chunks");

        ctable = (uint64_t*) ctxt->alloc_fn (chunkbytes);
        if (ctable == NULL)
            return ctxt->standard_error (
                (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);

        rv = ctxt->do_read (
            ctxt,
            ctable,
            chunkbytes,
            &chunkoff,
            &nread,
            EXR_MUST_READ_ALL);
        if (rv != EXR_ERR_SUCCESS)
        {
            ctxt->free_fn (ctable);
            return rv;
        }
        priv_to_native64 (ctable, part->chunk_count);
        //EXR_GETFILE(f)->report_error( ctxt, EXR_ERR_UNKNOWN, "TODO: implement reconstructLineOffsets and similar" );
        nptr = (uintptr_t) ctable;
        // see if we win or not
        if (!atomic_compare_exchange_strong (&(part->chunk_table), &eptr, nptr))
        {
            ctxt->free_fn (ctable);
            ctable = (uint64_t*) eptr;
            if (ctable == NULL)
                return ctxt->standard_error (
                    (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);
        }
    }

    *chunktable = ctable;

    chanlist = part->channels->chlist;
    chans    = chanlist->num_channels;
    if (chans <= 5) { chandecodes = cinfo->chan_store; }
    else
    {
        chandecodes =
            ctxt->alloc_fn (chans * sizeof (exr_channel_decode_info_t));
        if (chandecodes == NULL)
            return ctxt->standard_error (
                (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);
    }

    cinfo->channels      = chandecodes;
    cinfo->channel_count = chans;
    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        //*(chandecodes + c) = nilcd;
        chandecodes[c].channel_name  = curc->name.str;
        chandecodes[c].bytes_per_pel = (curc->pixel_type == EXR_PIXEL_HALF) ? 2
                                                                            : 4;
        chandecodes[c].x_samples     = curc->x_sampling;
        chandecodes[c].y_samples     = curc->y_sampling;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_destroy_decode_chunk_info (
    const exr_context_t ctxt, exr_decode_chunk_info_t* cinfo)
{
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);

    if (cinfo)
    {
        exr_decode_chunk_info_t nil = { 0 };
        if (cinfo->channels && cinfo->channels != cinfo->chan_store)
            pctxt->free_fn (cinfo->channels);

        if (cinfo->own_scratch_buffers)
        {
            if (cinfo->packed.buffer) pctxt->free_fn (cinfo->packed.buffer);
            if (cinfo->unpacked.buffer) pctxt->free_fn (cinfo->unpacked.buffer);
            if (cinfo->sample_table.buffer)
                pctxt->free_fn (cinfo->sample_table.buffer);
        }

        *cinfo = nil;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_decode_chunk_init_scanline (
    const exr_context_t      ctxt,
    int                      part_index,
    exr_decode_chunk_info_t* cinfo,
    int                      y,
    int                      own_scratch_space)
{
    uint64_t*        ctable;
    int              miny, cidx, rdcnt, lpc;
    int              data[3];
    int64_t          ddata[3];
    uint64_t         dataoff;
    exr_attr_box2i_t dw;
    exr_result_t     rv;

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    rv = initialize_part_read (pctxt, part, cinfo, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    lpc = part->lines_per_chunk;
    dw  = part->data_window;

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->standard_error (ctxt, EXR_ERR_SCAN_TILE_MIXEDAPI);
    }

    if (y < dw.y_min || y > dw.y_max)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for scanline %d outside range of data window (%d - %d)",
            y,
            dw.y_min,
            dw.y_max);
    }

    cidx = (y - dw.y_min);
    if (lpc > 1) cidx /= lpc;

    if (part->lineorder == EXR_LINEORDER_DECREASING_Y)
    {
        cidx = part->chunk_count - (cidx + 1);
        miny = dw.y_max - (cidx + 1) * lpc;
    }
    else
    {
        miny = cidx * lpc + dw.y_min;
    }
    if (cidx < 0 || cidx >= part->chunk_count)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for scanline %d in chunk %d outside chunk count %d",
            y,
            cidx,
            part->chunk_count);
    }

    cinfo->chunk_idx         = cidx;
    cinfo->chunk_type        = (uint8_t) part->storage_mode;
    cinfo->chunk_compression = (uint8_t) part->comp_type;
    cinfo->start_y           = miny;
    cinfo->height            = lpc;
    if (miny < dw.y_min)
    {
        cinfo->start_y = dw.y_min;
        cinfo->height -= (dw.y_min - miny);
    }
    else if ((miny + lpc) > dw.y_max)
    {
        cinfo->height = (dw.y_max - miny + 1);
    }
    cinfo->width               = dw.x_max - dw.x_min + 1;
    cinfo->own_scratch_buffers = own_scratch_space;
    for (int c = 0; c < cinfo->channel_count; ++c)
    {
        exr_channel_decode_info_t* curc = (cinfo->channels + c);
        if (curc->y_samples > 1)
        {
            if (cinfo->height == 1)
                curc->height = ((miny % curc->y_samples) == 0) ? 1 : 0;
            else
                curc->height = cinfo->height / curc->y_samples;
        }
        else
            curc->height = cinfo->height;

        if (curc->x_samples > 1)
            curc->width = cinfo->width / curc->x_samples;
        else
            curc->width = cinfo->width;
    }

    /* need to read from the file to get the packed chunk size */
    rdcnt = (pctxt->is_multipart) ? 2 : 1;
    if (part->storage_mode != EXR_STORAGE_DEEP_SCANLINE) ++rdcnt;

    dataoff = ctable[cidx];
    rv      = pctxt->do_read (
        pctxt,
        data,
        rdcnt * sizeof (int32_t),
        &dataoff,
        NULL,
        EXR_MUST_READ_ALL);
    if (rv != EXR_ERR_SUCCESS)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return rv;
    }
    priv_to_native32 (data, rdcnt);

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        rv = pctxt->do_read (
            pctxt,
            ddata,
            3 * sizeof (int64_t),
            &dataoff,
            NULL,
            EXR_MUST_READ_ALL);
        if (rv != EXR_ERR_SUCCESS)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return rv;
        }
        priv_to_native64 (ddata, 3);
    }
    cinfo->chunk_data_offset = dataoff;

    rdcnt = 0;
    if (pctxt->is_multipart)
    {
        if (data[rdcnt] != part_index)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for scanline %d found mismatch part %d vs %d in file",
                y,
                data[rdcnt],
                part_index);
        }
        ++rdcnt;
    }
    if (miny != data[rdcnt])
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for scanline %d found scanline %d, not %d at chunk %d",
            y,
            data[rdcnt],
            miny,
            cidx);
    }
    ++rdcnt;

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        cinfo->sample_table.size = ddata[0];
        cinfo->packed.size       = ddata[1];
        cinfo->unpacked.size     = ddata[2];
        cinfo->spare.size        = ddata[2];

        if (ddata[0] < 0)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d invalid sample table size %ld",
                y,
                ddata[0]);
        }
        if (ddata[1] < 0 || ddata[1] > (int64_t) INT_MAX)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d packed size %ld not supported",
                y,
                ddata[1]);
        }
        if (ddata[2] < 0 || ddata[2] > (int64_t) INT_MAX)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d unpacked size %ld not supported",
                y,
                ddata[2]);
        }
    }
    else
    {
        if (data[rdcnt] < 0 ||
            (uint64_t) data[rdcnt] > part->unpacked_size_per_chunk)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for scanline %d found invalid packed data block size %d",
                y,
                data[rdcnt]);
        }
        cinfo->packed.size   = data[rdcnt];
        cinfo->unpacked.size = part->unpacked_size_per_chunk;
        cinfo->spare.size    = part->unpacked_size_per_chunk;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
compute_tile_chunk_off (
    struct _internal_exr_context* ctxt,
    struct _internal_exr_part*    part,
    int                           tilex,
    int                           tiley,
    int                           levelx,
    int                           levely,
    int32_t*                      chunkoffout)
{
    int                        numx, numy;
    int64_t                    chunkoff = 0;
    const exr_attr_tiledesc_t* tiledesc = part->tiles->tiledesc;

    switch (EXR_GET_TILE_LEVEL_MODE ((*tiledesc)))
    {
        case EXR_TILE_ONE_LEVEL:
        case EXR_TILE_MIPMAP_LEVELS:
            if (levelx != levely)
            {
                return ctxt->print_error (
                    (const exr_context_t) ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Request for tile (%d, %d) level (%d, %d), but single level and mipmap tiles must have same level x and y",
                    tilex,
                    tiley,
                    levelx,
                    levely);
            }
            if (levelx >= part->num_tile_levels_x)
            {
                return ctxt->print_error (
                    (const exr_context_t) ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Request for tile (%d, %d) level %d, but level past available levels (%d)",
                    tilex,
                    tiley,
                    levelx,
                    part->num_tile_levels_x);
            }

            numx = part->tile_level_tile_count_x[levelx];
            numy = part->tile_level_tile_count_y[levelx];

            if (tilex >= numx || tiley >= numy)
            {
                return ctxt->print_error (
                    (const exr_context_t) ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Request for tile (%d, %d) level %d, but level only has %d x %d tiles",
                    tilex,
                    tiley,
                    levelx,
                    numx,
                    numy);
            }

            for (int l = 0; l < levelx; ++l)
                chunkoff +=
                    ((int64_t) part->tile_level_tile_count_x[l] *
                     (int64_t) part->tile_level_tile_count_y[l]);
            chunkoff += tiley * numx + tilex;
            break;

        case EXR_TILE_RIPMAP_LEVELS:
            if (levelx >= part->num_tile_levels_x)
            {
                return ctxt->print_error (
                    (const exr_context_t) ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Request for tile (%d, %d) level %d, %d, but x level past available levels (%d)",
                    tilex,
                    tiley,
                    levelx,
                    levely,
                    part->num_tile_levels_x);
            }
            if (levely >= part->num_tile_levels_y)
            {
                return ctxt->print_error (
                    (const exr_context_t) ctxt,
                    EXR_ERR_INVALID_ARGUMENT,
                    "Request for tile (%d, %d) level %d, %d, but y level past available levels (%d)",
                    tilex,
                    tiley,
                    levelx,
                    levely,
                    part->num_tile_levels_y);
            }

            // TODO
            return ctxt->print_error (
                (const exr_context_t) ctxt,
                EXR_ERR_UNKNOWN,
                "RIPMAP support not yet finished in C layer");
            break;
        default:
            return ctxt->print_error (
                (const exr_context_t) ctxt,
                EXR_ERR_UNKNOWN,
                "Invalid tile description");
    }

    if (chunkoff >= part->chunk_count)
    {
        return ctxt->print_error (
            (const exr_context_t) ctxt,
            EXR_ERR_UNKNOWN,
            "Invalid tile chunk offset %ld (%d avail)",
            chunkoff,
            part->chunk_count);
    }

    *chunkoffout = (int32_t) chunkoff;
    return EXR_ERR_SUCCESS;
}

exr_result_t
exr_decode_chunk_init_tile (
    exr_context_t            ctxt,
    int                      part_index,
    exr_decode_chunk_info_t* cinfo,
    int                      tilex,
    int                      tiley,
    int                      levelx,
    int                      levely,
    int                      own_scratch_space)
{
    int32_t                    data[6];
    int32_t*                   tdata = data;
    int32_t                    cidx = 0, ntoread = 5;
    uint64_t                   dataoff;
    int64_t                    fsize;
    const exr_attr_tiledesc_t* tiledesc;
    int                        tilew, tileh, unpacksize = 0;
    uint64_t*                  ctable;
    exr_result_t               rv;

    EXR_PROMOTE_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    rv = initialize_part_read (pctxt, part, cinfo, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
    }

    if (!part->tiles || part->num_tile_levels_x <= 0 ||
        part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
        !part->tile_level_tile_count_y)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile, but no tile data exists");
    }

    rv = compute_tile_chunk_off (
        pctxt, part, tilex, tiley, levelx, levely, &cidx);
    if (rv != EXR_ERR_SUCCESS)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return rv;
    }

    tiledesc = part->tiles->tiledesc;
    tilew    = part->tile_level_tile_size_x[levelx];
    if (tiledesc->x_size < tilew) tilew = tiledesc->x_size;
    tileh = part->tile_level_tile_size_y[levely];
    if (tiledesc->y_size < tileh) tileh = tiledesc->y_size;

    cinfo->chunk_idx           = cidx;
    cinfo->chunk_type          = (uint8_t) part->storage_mode;
    cinfo->chunk_compression   = (uint8_t) part->comp_type;
    cinfo->own_scratch_buffers = own_scratch_space;

    cinfo->start_y = tileh * tiley;
    cinfo->height  = tileh;
    cinfo->width   = tilew;
    for (int c = 0; c < cinfo->channel_count; ++c)
    {
        exr_channel_decode_info_t* curc = (cinfo->channels + c);
        curc->height                    = tileh;
        curc->width                     = tilew;
        unpacksize += tilew * tileh * curc->bytes_per_pel;
    }

    if (pctxt->is_multipart) ++ntoread;
    if (part->storage_mode == EXR_STORAGE_DEEP_TILED) --ntoread;
    if (pctxt->is_multipart) ++ntoread;

    dataoff = ctable[cidx];
    rv      = pctxt->do_read (
        pctxt,
        data,
        ntoread * sizeof (int32_t),
        &dataoff,
        &fsize,
        EXR_MUST_READ_ALL);
    if (rv != EXR_ERR_SUCCESS)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile (%d, %d), level (%d, %d) but unable to read %ld bytes from offset %ld, got %ld bytes",
            tilex,
            tiley,
            levelx,
            levely,
            ntoread * sizeof (int32_t),
            ctable[cidx],
            fsize);
    }

    priv_to_native32 (data, ntoread);
    if (ntoread == 6)
    {
        if (part_index != data[0])
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for multi-part tile found bad part index (%d), expect %d",
                data[0],
                part_index);
        }
        ++tdata;
    }
    if (tdata[0] != tilex)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile x coord mismatch: found %d, expect %d",
            tdata[0],
            tilex);
    }
    if (tdata[1] != tiley)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile y coord mismatch: found %d, expect %d",
            tdata[1],
            tiley);
    }
    if (tdata[2] != levelx)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile level x mismatch: found %d, expect %d",
            tdata[2],
            levelx);
    }
    if (tdata[3] != levely)
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile level y mismatch: found %d, expect %d",
            tdata[3],
            levely);
    }

    fsize = pctxt->file_size;
    if (part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        int64_t ddata[3];
        rv = pctxt->do_read (
            pctxt,
            ddata,
            3 * sizeof (int64_t),
            &dataoff,
            NULL,
            EXR_MUST_READ_ALL);
        if (rv != EXR_ERR_SUCCESS)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return rv;
        }
        priv_to_native64 (ddata, 3);

        if (ddata[0] < 0)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid sample table size %ld",
                tilex,
                tiley,
                levelx,
                levely,
                ddata[0]);
        }
        if (ddata[1] < 0 || ddata[1] > (int64_t) INT_MAX)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid packed size %ld",
                tilex,
                tiley,
                levelx,
                levely,
                ddata[1]);
        }
        if (ddata[2] < 0 || ddata[2] > (int64_t) INT_MAX)
        {
            exr_destroy_decode_chunk_info (ctxt, cinfo);
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid unpacked size %ld",
                tilex,
                tiley,
                levelx,
                levely,
                ddata[1]);
        }

        cinfo->sample_table.size = ddata[0];
        cinfo->packed.size       = ddata[1];
        cinfo->unpacked.size     = ddata[2];
        cinfo->spare.size        = ddata[2];
    }
    else if (
        tdata[4] < 0 || tdata[4] > unpacksize ||
        (fsize > 0 && tdata[4] > fsize))
    {
        exr_destroy_decode_chunk_info (ctxt, cinfo);
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Invalid data size found for tile (%d, %d) at level (%d, %d): %d unpack size %d file size %ld",
            tilex,
            tiley,
            levelx,
            levely,
            tdata[4],
            unpacksize,
            fsize);
    }
    else
    {
        cinfo->packed.size       = tdata[4];
        cinfo->unpacked.size     = unpacksize;
        cinfo->spare.size        = unpacksize;
        cinfo->chunk_data_offset = dataoff;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int
read_uncompressed_direct (
    const struct _internal_exr_context* ctxt, exr_decode_chunk_info_t* cinfo)
{
    uint64_t dataoffset = cinfo->chunk_data_offset;
    uint64_t nToRead;
    uint8_t* cdata;
    int      rv = EXR_ERR_SUCCESS;

    for (int y = 0; y < cinfo->height; ++y)
    {
        int cury = y + cinfo->start_y;
        for (int c = 0; c < cinfo->channel_count; ++c)
        {
            cdata = cinfo->channels[c].data_ptr;
            nToRead =
                cinfo->channels[c].width * cinfo->channels[c].bytes_per_pel;

            if (cinfo->channels[c].height == 0) continue;

            if (cinfo->channels[c].y_samples > 1)
            {
                if ((cury % cinfo->channels[c].y_samples) != 0) continue;
                cdata +=
                    ((uint64_t) (y / cinfo->channels[c].y_samples) *
                     (uint64_t) cinfo->channels[c].output_line_stride);
            }
            else
            {
                cdata += (uint64_t) y *
                         (uint64_t) cinfo->channels[c].output_line_stride;
            }

            /* actual read into the output pointer */
            rv = ctxt->do_read (
                ctxt,
                cdata,
                nToRead,
                &dataoffset,
                NULL,
                EXR_MUST_READ_ALL);
            if (rv != EXR_ERR_SUCCESS) return rv;

            // need to swab them to native
            if (cinfo->channels[c].bytes_per_pel == 2)
                priv_to_native16 (cdata, cinfo->channels[c].width);
            else
                priv_to_native32 (cdata, cinfo->channels[c].width);
        }
    }

    return rv;
}

/**************************************/

static void
unpack_16bit_4_chans (
    exr_decode_chunk_info_t* cinfo, const uint8_t* unpackbuffer)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t*  srcbuffer = unpackbuffer;
    const uint16_t *in0, *in1, *in2, *in3;
    uint8_t *       out0, *out1, *out2, *out3;
    int             w;
    int             inc0, inc1, inc2, inc3;
    int             linc0, linc1, linc2, linc3;
    w     = cinfo->channels[0].width;
    inc0  = cinfo->channels[0].output_pixel_stride;
    inc1  = cinfo->channels[1].output_pixel_stride;
    inc2  = cinfo->channels[2].output_pixel_stride;
    inc3  = cinfo->channels[3].output_pixel_stride;
    linc0 = cinfo->channels[0].output_line_stride;
    linc1 = cinfo->channels[1].output_line_stride;
    linc2 = cinfo->channels[2].output_line_stride;
    linc3 = cinfo->channels[3].output_line_stride;

    out0 = cinfo->channels[0].data_ptr;
    out1 = cinfo->channels[1].data_ptr;
    out2 = cinfo->channels[2].data_ptr;
    out3 = cinfo->channels[3].data_ptr;

    if (inc0 == 8 && inc0 == inc1 && inc0 == inc2 && inc0 == inc3 &&
        linc0 == linc1 && linc0 == linc2 && linc0 == linc3 &&
        (out0 + 2) == out1 && (out0 + 4) == out2 && (out0 + 6) == out3)
    {
        /* TODO: can do this with sse and do 2 outpixels at once */
        union
        {
            struct
            {
                uint16_t a;
                uint16_t b;
                uint16_t g;
                uint16_t r;
            };
            uint64_t allc;
        } combined;
        /* interleaving case, we can do this! */
        for (int y = 0; y < cinfo->height; ++y)
        {
            uint64_t* outall = (uint64_t*) out0;
            in0              = (uint16_t*) srcbuffer;
            in1              = (const uint16_t*) srcbuffer + w * 2;
            in2              = (const uint16_t*) srcbuffer + w * 4;
            in3              = (const uint16_t*) srcbuffer + w * 6;

            srcbuffer += w * 8;
            for (int x = 0; x < w; ++x)
            {
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
                combined.a = le16toh (in0[x]);
                combined.b = le16toh (in1[x]);
                combined.g = le16toh (in2[x]);
                combined.r = le16toh (in3[x]);
#else
                combined.a = in0[x];
                combined.b = in1[x];
                combined.g = in2[x];
                combined.r = in3[x];
#endif
                outall[x] = combined.allc;
            }
            out0 += linc0;
        }
    }
    else if (inc0 == 2 && inc1 == 2 && inc2 == 2 && inc3 == 2)
    {
        // planar output
        for (int y = 0; y < cinfo->height; ++y)
        {
            in0 = (uint16_t*) srcbuffer;
            in1 = (const uint16_t*) srcbuffer + w * 2;
            in2 = (const uint16_t*) srcbuffer + w * 4;
            in3 = (const uint16_t*) srcbuffer + w * 6;
            srcbuffer += w * 8;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            for (int x = 0; x < w; ++x)
                *(((uint16_t*) out0) + x) = le16toh (in0[x]);
            for (int x = 0; x < w; ++x)
                *(((uint16_t*) out1) + x) = le16toh (in1[x]);
            for (int x = 0; x < w; ++x)
                *(((uint16_t*) out2) + x) = le16toh (in2[x]);
            for (int x = 0; x < w; ++x)
                *(((uint16_t*) out3) + x) = le16toh (in3[x]);
#else
            memcpy (out0, in0, w * 2);
            memcpy (out1, in1, w * 2);
            memcpy (out2, in2, w * 2);
            memcpy (out3, in3, w * 2);
#endif
            out0 += linc0;
            out1 += linc1;
            out2 += linc2;
            out3 += linc3;
        }
    }
    else
    {
        for (int y = 0; y < cinfo->height; ++y)
        {
            in0 = (uint16_t*) srcbuffer;
            in1 = (const uint16_t*) srcbuffer + w * 2;
            in2 = (const uint16_t*) srcbuffer + w * 4;
            in3 = (const uint16_t*) srcbuffer + w * 6;
            srcbuffer += w * 8;
            for (int x = 0; x < w; ++x)
                *((uint16_t*) (out0 + x * inc0)) = in0[x];
            for (int x = 0; x < w; ++x)
                *((uint16_t*) (out1 + x * inc1)) = in1[x];
            for (int x = 0; x < w; ++x)
                *((uint16_t*) (out2 + x * inc2)) = in2[x];
            for (int x = 0; x < w; ++x)
                *((uint16_t*) (out3 + x * inc3)) = in3[x];
            out0 += linc0;
            out1 += linc1;
            out2 += linc2;
            out3 += linc3;
        }
    }
}

/**************************************/

static void
unpack_16bit_all_chans (
    exr_decode_chunk_info_t* cinfo, const uint8_t* unpackbuffer)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t* srcbuffer = unpackbuffer;
    uint8_t*       cdata;
    int            w, bpc, pixincrement;
    for (int y = 0; y < cinfo->height; ++y)
    {
        for (int c = 0; c < cinfo->channel_count; ++c)
        {
            cdata        = cinfo->channels[c].data_ptr;
            w            = cinfo->channels[c].width;
            pixincrement = cinfo->channels[c].output_pixel_stride;
            cdata +=
                (uint64_t) y * (uint64_t) cinfo->channels[c].output_line_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            if (pixincrement == 2)
            {
                uint16_t*       tmp = (uint16_t*) cdata;
                const uint16_t* src = (const uint16_t*) srcbuffer;
                uint16_t*       end = tmp + w;

                while (tmp < end)
                    *tmp++ = le16toh (*src++);
            }
            else
            {
                const uint16_t* src = (const uint16_t*) srcbuffer;
                for (int x = 0; x < w; ++x)
                {
                    *((uint16_t*) cdata) = le16toh (*src++);
                    cdata += pixincrement;
                }
            }
#else
            if (pixincrement == 2)
            {
                memcpy (cdata, srcbuffer, w * pixincrement);
            }
            else
            {
                const uint16_t* src = (const uint16_t*) srcbuffer;
                for (int x = 0; x < w; ++x)
                {
                    *((uint16_t*) cdata) = *src++;
                    cdata += pixincrement;
                }
            }
#endif
            srcbuffer += w * 2;
        }
    }
}

/**************************************/

static void
unpack_32bit_all_chans (
    exr_decode_chunk_info_t* cinfo, const uint8_t* unpackbuffer)
{
    /* we know we're unpacking all the channels and there is no subsampling */
    const uint8_t* srcbuffer = unpackbuffer;
    uint8_t*       cdata;
    int64_t        w, h, pixincrement;
    int            chans = cinfo->channel_count;
    h                    = (int64_t) cinfo->height;
    for (int64_t y = 0; y < h; ++y)
    {
        for (int c = 0; c < chans; ++c)
        {
            cdata        = cinfo->channels[c].data_ptr;
            w            = cinfo->channels[c].width;
            pixincrement = cinfo->channels[c].output_pixel_stride;
            cdata += y * (int64_t) cinfo->channels[c].output_line_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
            if (pixincrement == 4)
            {
                uint32_t*       tmp = (uint32_t*) cdata;
                const uint32_t* src = (const uint32_t*) srcbuffer;
                uint32_t*       end = tmp + w;

                while (tmp < end)
                    *tmp++ = le32toh (*src++);
            }
            else
            {
                const uint32_t* src = (const uint32_t*) srcbuffer;
                for (int64_t x = 0; x < w; ++x)
                {
                    *((uint32_t*) cdata) = le32toh (*src++);
                    cdata += pixincrement;
                }
            }
#else
            if (pixincrement == 4)
            {
                memcpy (cdata, srcbuffer, w * pixincrement);
            }
            else
            {
                const uint32_t* src = (const uint32_t*) srcbuffer;
                for (int64_t x = 0; x < w; ++x)
                {
                    *((uint32_t*) cdata) = *src++;
                    cdata += pixincrement;
                }
            }
#endif
            srcbuffer += w * 4;
        }
    }
}

/**************************************/

static void
generic_unpack_subsampled (
    exr_decode_chunk_info_t* cinfo, const uint8_t* unpackbuffer)
{
    const uint8_t* srcbuffer = unpackbuffer;
    uint8_t*       cdata;
    int            w, bpc;
    for (int y = 0; y < cinfo->height; ++y)
    {
        int cury = y + cinfo->start_y;
        for (int c = 0; c < cinfo->channel_count; ++c)
        {
            cdata = cinfo->channels[c].data_ptr;
            w     = cinfo->channels[c].width;
            bpc   = cinfo->channels[c].bytes_per_pel;

            if (cinfo->channels[c].y_samples > 1)
            {
                if ((cury % cinfo->channels[c].y_samples) != 0) continue;
                if (cdata)
                    cdata +=
                        ((uint64_t) (y / cinfo->channels[c].y_samples) *
                         (uint64_t) cinfo->channels[c].output_line_stride);
            }
            else if (cdata)
            {
                cdata += (uint64_t) y *
                         (uint64_t) cinfo->channels[c].output_line_stride;
            }

            if (cdata)
            {
                int pixincrement = cinfo->channels[c].output_pixel_stride;
#if EXR_HOST_IS_NOT_LITTLE_ENDIAN
                if (bpc == 2)
                {
                    if (pixincrement == 2)
                    {
                        uint16_t*       tmp = (uint16_t*) cdata;
                        const uint16_t* src = (const uint16_t*) srcbuffer;
                        uint16_t*       end = tmp + w;

                        while (tmp < end)
                            *tmp++ = le16toh (*src++);
                    }
                    else
                    {
                        const uint16_t* src = (const uint16_t*) srcbuffer;
                        for (int x = 0; x < w; ++x)
                        {
                            *((uint16_t*) cdata) = le16toh (*src++);
                            cdata += pixincrement;
                        }
                    }
                }
                else
                {
                    if (pixincrement == 4)
                    {
                        uint32_t*       tmp = (uint32_t*) cdata;
                        const uint32_t* src = (const uint32_t*) srcbuffer;
                        uint32_t*       end = tmp + w;

                        while (tmp < end)
                            *tmp++ = le32toh (*src++);
                    }
                    else
                    {
                        const uint32_t* src = (const uint32_t*) srcbuffer;
                        for (int x = 0; x < w; ++x)
                        {
                            *((uint32_t*) cdata) = le32toh (*src++);
                            cdata += pixincrement;
                        }
                    }
                }
#else
                if (bpc == pixincrement)
                {
                    memcpy (cdata, srcbuffer, w * pixincrement);
                }
                else if (bpc == 2)
                {
                    const uint16_t* src = (const uint16_t*) srcbuffer;
                    for (int x = 0; x < w; ++x)
                    {
                        *((uint16_t*) cdata) = *src++;
                        cdata += pixincrement;
                    }
                }
                else if (bpc == 4)
                {
                    const uint32_t* src = (const uint32_t*) srcbuffer;
                    for (int x = 0; x < w; ++x)
                    {
                        *((uint32_t*) cdata) = *src++;
                        cdata += pixincrement;
                    }
                }
#endif /* byte order check */
            }
            srcbuffer += w * bpc;
        }
    }
}

/**************************************/

static inline void
unpack_data (
    exr_decode_chunk_info_t* cinfo,
    const uint8_t*           unpackbuffer,
    int                      chanstofill,
    int                      samebpc,
    int                      hassampling)
{
    const uint8_t* srcbuffer = unpackbuffer;
    uint8_t*       cdata;
    int            w, bpc;

    if (hassampling || chanstofill != cinfo->channel_count || samebpc <= 0)
    {
        generic_unpack_subsampled (cinfo, unpackbuffer);
    }
    else if (samebpc == 2)
    {
        if (cinfo->channel_count == 4)
            unpack_16bit_4_chans (cinfo, unpackbuffer);
        else
            unpack_16bit_all_chans (cinfo, unpackbuffer);
    }
    else
    {
        unpack_32bit_all_chans (cinfo, unpackbuffer);
    }
}

/**************************************/

exr_result_t
exr_read_chunk (exr_context_t ctxt, exr_decode_chunk_info_t* cinfo)
{
    int      chanstofill = 0, chanstounpack = 0, samebpc = 0, hassampling = 0;
    uint64_t dataoffset;
    uint64_t nToRead;
    uint8_t* cdata;
    exr_result_t rv = EXR_ERR_SUCCESS;

    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);

    if (!cinfo)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Missing chunk info pointer to read chunk");

    dataoffset = cinfo->chunk_data_offset;
    for (int c = 0; c < cinfo->channel_count; ++c)
    {
        cdata = cinfo->channels[c].data_ptr;
        if (cinfo->channels[c].height == 0 || !cdata) continue;

        if (samebpc == 0)
            samebpc = cinfo->channels[c].bytes_per_pel;
        else if (samebpc != cinfo->channels[c].bytes_per_pel)
            samebpc = -1;

        if (cinfo->channels[c].x_samples != 1 ||
            cinfo->channels[c].y_samples != 1)
            hassampling = 1;

        ++chanstofill;
        if (cinfo->channels[c].output_pixel_stride !=
            cinfo->channels[c].bytes_per_pel)
        {
            ++chanstounpack;
        }
    }

    /* special case, uncompressed and reading planar data straight in
     * to all the channels */
    if (cinfo->packed.size == cinfo->unpacked.size && chanstounpack == 0 &&
        chanstofill > 0 && chanstofill == cinfo->channel_count)
    {
        return read_uncompressed_direct (pctxt, cinfo);
    }

    // read the packed buffer in (might be uncompressed but byte swapped)
    if (!cinfo->packed.buffer)
    {
        if (cinfo->own_scratch_buffers)
        {
            cinfo->packed.buffer = pctxt->alloc_fn (cinfo->packed.size);
            if (!cinfo->packed.buffer)
            {
                return pctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
            }
        }
        else
        {
            return pctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
        }
    }
    cdata      = cinfo->packed.buffer;
    dataoffset = cinfo->chunk_data_offset;
    nToRead    = cinfo->packed.size;
    rv         = pctxt->do_read (
        pctxt, cdata, nToRead, &dataoffset, NULL, EXR_MUST_READ_ALL);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (cinfo->packed.size != cinfo->unpacked.size)
    {
        if (cinfo->chunk_compression == EXR_COMPRESSION_NONE)
            return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

        if (!cinfo->unpacked.buffer)
        {
            if (cinfo->own_scratch_buffers)
            {
                cinfo->unpacked.buffer = pctxt->alloc_fn (cinfo->unpacked.size);
                if (!cinfo->unpacked.buffer)
                {
                    return pctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
                }
            }
            else
            {
                /****** EXIT POINT ******/
                /* user said they own the buffers, and we've done what we can,
                 * they only want the compressed */
                return EXR_ERR_SUCCESS;
            }
        }
        rv = exr_decompress_data (
            ctxt,
            (exr_compression_t) cinfo->chunk_compression,
            cinfo->packed.buffer,
            cinfo->packed.size,
            cinfo->unpacked.buffer,
            cinfo->unpacked.size);

        if (chanstofill > 0)
            unpack_data (
                cinfo,
                cinfo->unpacked.buffer,
                chanstofill,
                samebpc,
                hassampling);
    }
    else if (chanstofill > 0)
        unpack_data (
            cinfo,
            cinfo->packed.buffer,
            chanstofill,
            samebpc,
            hassampling);

    return rv;
}
