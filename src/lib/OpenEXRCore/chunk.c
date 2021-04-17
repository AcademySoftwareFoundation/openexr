/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_chunkio.h"

#include "internal_structs.h"
#include "internal_xdr.h"

#include <limits.h>
#include <string.h>

/**************************************/

/* for testing, we include a bunch of internal stuff into the unit tests which are in c++ */
#if defined __has_include
#    if __has_include(<stdatomic.h>)
#        define EXR_HAS_STD_ATOMICS 1
#    endif
#endif

#ifdef EXR_HAS_STD_ATOMICS
#    include <stdatomic.h>
#elif defined(_MSC_VER)

/* msvc w/ c11 support is only very new, until we know what the preprocessor checks are, provide defaults */
#    include <windows.h>

#    define atomic_load(object) InterlockedOr64 ((int64_t volatile*) object, 0)

static inline int
atomic_compare_exchange_strong (
    uint64_t volatile* object, uint64_t* expected, uint64_t desired)
{
    uint64_t prev =
        (uint64_t) InterlockedCompareExchange64 (object, desired, *expected);
    if (prev == *expected) return 1;
    *expected = prev;
    return 0;
}

#else
#    error OS unimplemented support for atomics
#endif

/**************************************/

static exr_result_t
extract_chunk_table (
    const struct _internal_exr_context* ctxt,
    const struct _internal_exr_part*    part,
    uint64_t**                          chunktable)
{
    uint64_t*         ctable    = NULL;
    atomic_uintptr_t* ctableptr = (atomic_uintptr_t*) &(part->chunk_table);

    ctable = (uint64_t*) atomic_load (ctableptr);
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
            ctxt, ctable, chunkbytes, &chunkoff, &nread, EXR_MUST_READ_ALL);
        if (rv != EXR_ERR_SUCCESS)
        {
            ctxt->free_fn (ctable);
            return rv;
        }
        priv_to_native64 (ctable, part->chunk_count);

        //EXR_GETFILE(f)->report_error( ctxt, EXR_ERR_UNKNOWN, "TODO: implement reconstructLineOffsets and similar" );
        nptr = (uintptr_t) ctable;
        // see if we win or not
        if (!atomic_compare_exchange_strong (ctableptr, &eptr, nptr))
        {
            ctxt->free_fn (ctable);
            ctable = (uint64_t*) eptr;
            if (ctable == NULL)
                return ctxt->standard_error (
                    (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);
        }
    }

    *chunktable = ctable;
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
alloc_chunk_table (
    const struct _internal_exr_context* ctxt,
    const struct _internal_exr_part*    part,
    uint64_t**                          chunktable)
{
    uint64_t*         ctable    = NULL;
    atomic_uintptr_t* ctableptr = (atomic_uintptr_t*) &(part->chunk_table);

    /* we have the lock, but to access the type, we'll use the atomic function anyway */
    ctable = (uint64_t*) atomic_load (ctableptr);
    if (ctable == NULL)
    {
        uint64_t  chunkbytes = sizeof (uint64_t) * (uint64_t) part->chunk_count;
        uintptr_t eptr = 0, nptr = 0;

        ctable = (uint64_t*) ctxt->alloc_fn (chunkbytes);
        if (ctable == NULL)
            return ctxt->standard_error (
                (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);

        if (!atomic_compare_exchange_strong (ctableptr, &eptr, nptr))
        {
            ctxt->free_fn (ctable);
            ctable = (uint64_t*) eptr;
            if (ctable == NULL)
                return ctxt->standard_error (
                    (const exr_context_t) ctxt, EXR_ERR_OUT_OF_MEMORY);
        }
        memset (ctable, 0, chunkbytes);
    }
    *chunktable = ctable;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_read_scanline_block_info (
    const exr_context_t     ctxt,
    int                     part_index,
    int                     y,
    exr_chunk_block_info_t* cinfo)
{
    exr_result_t     rv;
    int              miny, cidx, rdcnt, lpc;
    int32_t          data[3];
    int64_t          ddata[3];
    uint64_t         dataoff;
    exr_attr_box2i_t dw;
    uint64_t*        ctable;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!cinfo) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        return pctxt->standard_error (ctxt, EXR_ERR_SCAN_TILE_MIXEDAPI);
    }

    dw = part->data_window;
    if (y < dw.y_min || y > dw.y_max)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for scanline %d outside range of data window (%d - %d)",
            y,
            dw.y_min,
            dw.y_max);
    }

    lpc  = part->lines_per_chunk;
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
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for scanline %d in chunk %d outside chunk count %d",
            y,
            cidx,
            part->chunk_count);
    }

    cinfo->idx         = cidx;
    cinfo->type        = (uint8_t) part->storage_mode;
    cinfo->compression = (uint8_t) part->comp_type;
    cinfo->start_y     = miny;
    cinfo->width       = dw.x_max - dw.x_min + 1;
    cinfo->height      = lpc;
    if (miny < dw.y_min)
    {
        cinfo->start_y = dw.y_min;
        cinfo->height -= (dw.y_min - miny);
    }
    else if ((miny + lpc) > dw.y_max)
    {
        cinfo->height = (dw.y_max - miny + 1);
    }

    /* need to read from the file to get the packed chunk size */
    rv = extract_chunk_table (pctxt, part, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    dataoff = ctable[cidx];
    /* multi part files have the part for validation */
    rdcnt = (pctxt->is_multipart) ? 2 : 1;
    /* deep has 64-bit data, so be variable about what we read */
    if (part->storage_mode != EXR_STORAGE_DEEP_SCANLINE) ++rdcnt;

    rv = pctxt->do_read (
        pctxt,
        data,
        rdcnt * sizeof (int32_t),
        &dataoff,
        NULL,
        EXR_MUST_READ_ALL);

    if (rv != EXR_ERR_SUCCESS) return rv;

    priv_to_native32 (data, rdcnt);

    rdcnt = 0;
    if (pctxt->is_multipart)
    {
        if (data[rdcnt] != part_index)
        {
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
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for scanline %d found scanline %d, not %d at chunk %d",
            y,
            data[rdcnt],
            miny,
            cidx);
    }

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        rv = pctxt->do_read (
            pctxt,
            ddata,
            3 * sizeof (int64_t),
            &dataoff,
            NULL,
            EXR_MUST_READ_ALL);
        if (rv != EXR_ERR_SUCCESS) { return rv; }
        priv_to_native64 (ddata, 3);

        if (ddata[0] < 0)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d invalid sample table size %" PRId64,
                y,
                ddata[0]);
        }
        if (ddata[1] < 0 || ddata[1] > (int64_t) INT_MAX)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d large packed size %" PRId64
                " not supported",
                y,
                ddata[1]);
        }
        if (ddata[2] < 0 || ddata[2] > (int64_t) INT_MAX)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep scanline %d large unpacked size %" PRId64
                " not supported",
                y,
                ddata[2]);
        }

        cinfo->sample_count_data_offset = dataoff;
        cinfo->sample_count_table_size  = ddata[0];
        cinfo->data_offset              = dataoff + ddata[0];
        cinfo->packed_size              = ddata[1];
        cinfo->unpacked_size            = ddata[2];
    }
    else
    {
        ++rdcnt;
        if (data[rdcnt] < 0 ||
            (uint64_t) data[rdcnt] > part->unpacked_size_per_chunk)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for scanline %d found invalid packed data block size %d",
                y,
                data[rdcnt]);
        }

        cinfo->data_offset              = dataoff;
        cinfo->packed_size              = data[rdcnt];
        cinfo->unpacked_size            = part->unpacked_size_per_chunk;
        cinfo->sample_count_data_offset = 0;
        cinfo->sample_count_table_size  = 0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
compute_tile_chunk_off (
    const struct _internal_exr_context* ctxt,
    const struct _internal_exr_part*    part,
    int                                 tilex,
    int                                 tiley,
    int                                 levelx,
    int                                 levely,
    int32_t*                            chunkoffout)
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
            "Invalid tile chunk offset %" PRId64 " (%d avail)",
            chunkoff,
            part->chunk_count);
    }

    *chunkoffout = (int32_t) chunkoff;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_read_tile_block_info (
    const exr_context_t     ctxt,
    int                     part_index,
    int                     tilex,
    int                     tiley,
    int                     levelx,
    int                     levely,
    exr_chunk_block_info_t* cinfo)
{
    exr_result_t               rv;
    int32_t                    data[6];
    int32_t*                   tdata;
    int32_t                    cidx, ntoread;
    uint64_t                   dataoff;
    int64_t                    fsize;
    const exr_attr_chlist_t*   chanlist;
    const exr_attr_tiledesc_t* tiledesc;
    int                        tilew, tileh, unpacksize = 0;
    uint64_t*                  ctable;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!cinfo) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        return pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
    }

    if (!part->tiles || part->num_tile_levels_x <= 0 ||
        part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
        !part->tile_level_tile_count_y)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile, but no tile data exists");
    }

    tiledesc = part->tiles->tiledesc;
    tilew    = part->tile_level_tile_size_x[levelx];
    if (tiledesc->x_size < (uint32_t) tilew) tilew = tiledesc->x_size;
    tileh = part->tile_level_tile_size_y[levely];
    if (tiledesc->y_size < (uint32_t) tileh) tileh = tiledesc->y_size;

    cidx = 0;
    rv   = compute_tile_chunk_off (
        pctxt, part, tilex, tiley, levelx, levely, &cidx);
    if (rv != EXR_ERR_SUCCESS) return rv;

    cinfo->idx         = cidx;
    cinfo->type        = (uint8_t) part->storage_mode;
    cinfo->compression = (uint8_t) part->comp_type;
    cinfo->start_y     = tileh * tiley;
    cinfo->height      = tileh;
    cinfo->width       = tilew;

    chanlist = part->channels->chlist;
    for (int c = 0; c < chanlist->num_channels; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        //*(chandecodes + c) = nilcd;
        unpacksize +=
            tilew * tileh * ((curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4);
    }

    rv = extract_chunk_table (pctxt, part, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (pctxt->is_multipart && part->storage_mode != EXR_STORAGE_DEEP_TILED)
        ntoread = 6;
    else
        ntoread = 5;

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
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile (%d, %d), level (%d, %d) but unable to read %" PRId64
            " bytes from offset %" PRId64 ", got %" PRId64 " bytes",
            tilex,
            tiley,
            levelx,
            levely,
            ntoread * sizeof (int32_t),
            ctable[cidx],
            fsize);
    }
    priv_to_native32 (data, ntoread);

    tdata = data;
    if (pctxt->is_multipart)
    {
        if (part_index != data[0])
        {
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
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile x coord mismatch: found %d, expect %d",
            tdata[0],
            tilex);
    }
    if (tdata[1] != tiley)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile y coord mismatch: found %d, expect %d",
            tdata[1],
            tiley);
    }
    if (tdata[2] != levelx)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Request for tile found tile level x mismatch: found %d, expect %d",
            tdata[2],
            levelx);
    }
    if (tdata[3] != levely)
    {
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
        if (rv != EXR_ERR_SUCCESS) { return rv; }
        priv_to_native64 (ddata, 3);

        if (ddata[0] < 0)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid sample table size %" PRId64,
                tilex,
                tiley,
                levelx,
                levely,
                ddata[0]);
        }

        /* not all compressors support 64-bit */
        if (ddata[1] < 0 || ddata[1] > (int64_t) INT32_MAX)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid packed size %" PRId64,
                tilex,
                tiley,
                levelx,
                levely,
                ddata[1]);
        }
        if (ddata[2] < 0 || ddata[2] > (int64_t) INT32_MAX)
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) invalid unpacked size %" PRId64,
                tilex,
                tiley,
                levelx,
                levely,
                ddata[1]);
        }
        if (fsize > 0 && (ddata[0] > fsize || ddata[1] > fsize ||
                          (ddata[0] + ddata[1]) > fsize))
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Request for deep tile (%d, %d), level (%d, %d) table (%" PRId64
                ") and/or data (%" PRId64
                ") size larger than file size %" PRId64,
                tilex,
                tiley,
                levelx,
                levely,
                ddata[0],
                ddata[1],
                fsize);
        }

        cinfo->sample_count_data_offset = dataoff;
        cinfo->sample_count_table_size  = ddata[0];
        cinfo->packed_size              = ddata[1];
        cinfo->unpacked_size            = ddata[2];
        cinfo->data_offset              = dataoff + ddata[0];
    }
    else
    {
        if (tdata[4] < 0 || tdata[4] > unpacksize ||
            (fsize > 0 && tdata[4] > fsize))
        {
            return pctxt->print_error (
                ctxt,
                EXR_ERR_BAD_CHUNK_DATA,
                "Invalid data size found for tile (%d, %d) at level (%d, %d): %d unpack size %d file size %" PRId64,
                tilex,
                tiley,
                levelx,
                levely,
                tdata[4],
                unpacksize,
                fsize);
        }
        cinfo->packed_size              = tdata[4];
        cinfo->unpacked_size            = unpacksize;
        cinfo->data_offset              = dataoff;
        cinfo->sample_count_data_offset = 0;
        cinfo->sample_count_table_size  = 0;
    }
    return EXR_ERR_SUCCESS;
}

exr_result_t
exr_read_chunk (
    const exr_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    void*                         packed_data)
{
    exr_result_t                 rv;
    uint64_t                     dataoffset, toread;
    int64_t                      nread;
    enum _INTERNAL_EXR_READ_MODE rmode = EXR_MUST_READ_ALL;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!cinfo) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
    if (!packed_data)
        return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (cinfo->idx < 0 || cinfo->idx >= part->chunk_count)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "invalid chunk index (%d) vs part chunk count %d",
            cinfo->idx,
            part->chunk_count);
    if (cinfo->type != (uint8_t) part->storage_mode)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "mis-matched storage type for chunk block info");
    if (cinfo->compression != (uint8_t) part->comp_type)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "mis-matched compression type for chunk block info");

    dataoffset = cinfo->data_offset;
    if (pctxt->file_size > 0 && dataoffset > (uint64_t) pctxt->file_size)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "chunk block info data offset (%" PRIu64
            ") past end of file (%" PRId64 ")",
            dataoffset,
            pctxt->file_size);

    /* allow a short read if uncompressed */
    if (part->comp_type == EXR_COMPRESSION_NONE) rmode = EXR_ALLOW_SHORT_READ;

    toread = cinfo->packed_size;
    nread  = 0;
    rv =
        pctxt->do_read (pctxt, packed_data, toread, &dataoffset, &nread, rmode);

    if (rmode == EXR_ALLOW_SHORT_READ && nread < (int64_t) toread)
        memset (((uint8_t *)packed_data) + nread, 0, toread - nread);
    return rv;
}

/**************************************/

exr_result_t
exr_read_deep_chunk (
    const exr_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    void*                         packed_data,
    void*                         sample_data)
{
    exr_result_t                 rv;
    uint64_t                     dataoffset, toread;
    int64_t                      nread;
    enum _INTERNAL_EXR_READ_MODE rmode = EXR_MUST_READ_ALL;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!cinfo) return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
    if (!packed_data && !sample_data)
        return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    if (cinfo->idx < 0 || cinfo->idx >= part->chunk_count)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "invalid chunk index (%d) vs part chunk count %d",
            cinfo->idx,
            part->chunk_count);
    if (cinfo->type != (uint8_t) part->storage_mode)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "mis-matched storage type for chunk block info");
    if (cinfo->compression != (uint8_t) part->comp_type)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "mis-matched compression type for chunk block info");

    if (pctxt->file_size > 0 &&
        cinfo->sample_count_data_offset > (uint64_t) pctxt->file_size)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "chunk block info sample count offset (%" PRIu64
            ") past end of file (%" PRId64 ")",
            cinfo->sample_count_data_offset,
            pctxt->file_size);

    if (pctxt->file_size > 0 &&
        cinfo->data_offset > (uint64_t) pctxt->file_size)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "chunk block info data offset (%" PRIu64
            ") past end of file (%" PRId64 ")",
            cinfo->data_offset,
            pctxt->file_size);

    rv = EXR_ERR_SUCCESS;
    if (sample_data)
    {
        dataoffset = cinfo->sample_count_data_offset;
        toread     = cinfo->sample_count_table_size;
        nread      = 0;
        rv         = pctxt->do_read (
            pctxt, sample_data, toread, &dataoffset, &nread, rmode);
    }

    if (rv != EXR_ERR_SUCCESS) return rv;

    if (packed_data)
    {
        dataoffset = cinfo->data_offset;
        toread     = cinfo->packed_size;
        nread      = 0;
        rv         = pctxt->do_read (
            pctxt, packed_data, toread, &dataoffset, &nread, rmode);
    }

    return rv;
}

/**************************************/

/* pull most of the logic to here to avoid having to unlock at every
 * error exit point and re-use mostly shared logic */
static exr_result_t
write_scan_chunk (
    exr_context_t                 ctxt,
    int                           part_index,
    struct _internal_exr_context* pctxt,
    struct _internal_exr_part*    part,
    int                           y,
    const void*                   packed_data,
    size_t                        packed_size,
    size_t                        unpacked_size,
    const void*                   sample_data,
    size_t                        sample_data_size)
{
    exr_result_t rv;
    int32_t      data[3];
    int32_t      psize;
    int          cidx, lpc, miny, wrcnt;
    uint64_t*    ctable;

    if (pctxt->mode != EXR_CONTEXT_WRITING_DATA)
    {
        if (pctxt->mode == EXR_CONTEXT_WRITE)
            return pctxt->standard_error (ctxt, EXR_ERR_HEADER_NOT_WRITTEN);
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    }

    if (part->storage_mode == EXR_STORAGE_TILED ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        return pctxt->standard_error (ctxt, EXR_ERR_SCAN_TILE_MIXEDAPI);
    }

    if (pctxt->cur_output_part != part_index)
        return pctxt->standard_error (ctxt, EXR_ERR_PART_NOT_READY);

    if (!packed_data || packed_size == 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid packed data argument size %" PRIu64 " pointer %p",
            (uint64_t) packed_size,
            packed_data);

    if (part->storage_mode != EXR_STORAGE_DEEP_SCANLINE &&
        packed_size > (size_t) INT32_MAX)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Packed data size %" PRIu64 " too large (max %" PRIu64 ")",
            (uint64_t) packed_size,
            (uint64_t) INT32_MAX);
    psize = (int32_t) packed_size;

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE &&
        (!sample_data || sample_data_size == 0))
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid sample count data argument size %" PRIu64 " pointer %p",
            (uint64_t) sample_data_size,
            sample_data);

    if (y < part->data_window.y_min || y > part->data_window.y_max)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid attempt to write scanlines starting at %d outside range of data window (%d - %d)",
            y,
            part->data_window.y_min,
            part->data_window.y_max);
    }

    lpc  = part->lines_per_chunk;
    cidx = (y - part->data_window.y_min);
    if (lpc > 1) cidx /= lpc;

    if (part->lineorder == EXR_LINEORDER_DECREASING_Y)
    {
        cidx = part->chunk_count - (cidx + 1);
        miny = part->data_window.y_max - (cidx + 1) * lpc;
    }
    else
    {
        miny = cidx * lpc + part->data_window.y_min;
    }

    if (y != miny)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Attempt to write scanline %d which does not align with y dims (%d) for chunk index (%d)",
            y,
            miny,
            cidx);
    }

    if (cidx < 0 || cidx >= part->chunk_count)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Chunk index for scanline %d in chunk %d outside chunk count %d",
            y,
            cidx,
            part->chunk_count);
    }

    if (part->lineorder != EXR_LINEORDER_RANDOM_Y &&
        pctxt->last_output_chunk != (cidx - 1))
    {
        return pctxt->standard_error (ctxt, EXR_ERR_CHUNK_NOT_READY);
    }

    if (pctxt->is_multipart)
    {
        data[0] = part_index;
        data[1] = miny;
        if (part->storage_mode != EXR_STORAGE_DEEP_SCANLINE)
        {
            data[2] = psize;
            wrcnt   = 3;
        }
        else
            wrcnt = 2;
    }
    else
    {
        data[0] = miny;
        if (part->storage_mode != EXR_STORAGE_DEEP_SCANLINE)
        {
            data[1] = psize;
            wrcnt   = 2;
        }
        else
            wrcnt = 1;
    }
    priv_from_native32 (data, wrcnt);

    rv = alloc_chunk_table (pctxt, part, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    ctable[cidx] = pctxt->output_file_offset;
    rv           = pctxt->do_write (
        pctxt, data, wrcnt * sizeof (int32_t), &(pctxt->output_file_offset));
    if (rv == EXR_ERR_SUCCESS &&
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        int64_t ddata[3];
        ddata[0] = (uint64_t) sample_data_size;
        ddata[1] = (uint64_t) packed_size;
        ddata[2] = (uint64_t) unpacked_size;
        rv       = pctxt->do_write (
            pctxt, ddata, 3 * sizeof (uint64_t), &(pctxt->output_file_offset));

        if (rv == EXR_ERR_SUCCESS)
            rv = pctxt->do_write (
                pctxt,
                sample_data,
                sample_data_size,
                &(pctxt->output_file_offset));
    }
    if (rv == EXR_ERR_SUCCESS)
        rv = pctxt->do_write (
            pctxt, packed_data, packed_size, &(pctxt->output_file_offset));

    if (rv == EXR_ERR_SUCCESS)
    {
        ++(pctxt->output_chunk_count);
        if (pctxt->output_chunk_count == part->chunk_count)
        {
            uint64_t chunkoff = part->chunk_table_offset;

            ++(pctxt->cur_output_part);
            if (pctxt->cur_output_part == pctxt->num_parts)
                pctxt->mode = EXR_CONTEXT_WRITE_FINISHED;
            pctxt->last_output_chunk  = -1;
            pctxt->output_chunk_count = 0;

            priv_from_native64 (ctable, part->chunk_count);
            rv = pctxt->do_write (
                pctxt,
                ctable,
                sizeof (uint64_t) * part->chunk_count,
                &chunkoff);
            /* just in case we look at it again? */
            priv_to_native64 (ctable, part->chunk_count);
        }
        else
        {
            pctxt->last_output_chunk = cidx;
        }
    }

    return rv;
}

/**************************************/

exr_result_t
exr_write_scanline_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           y,
    const void*   packed_data,
    size_t        packed_size)
{
    exr_result_t rv;
    EXR_PROMOTE_LOCKED_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
        return EXR_UNLOCK (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_USE_SCAN_DEEP_WRITE);

    rv = write_scan_chunk (
        ctxt, part_index, pctxt, part, y, packed_data, packed_size, 0, NULL, 0);
    return EXR_UNLOCK (pctxt), rv;
}

/**************************************/

exr_result_t
exr_write_deep_scanline_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           y,
    const void*   packed_data,
    size_t        packed_size,
    size_t        unpacked_size,
    const void*   sample_data,
    size_t        sample_data_size)
{
    exr_result_t rv;
    EXR_PROMOTE_LOCKED_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_SCANLINE)
        return EXR_UNLOCK (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_USE_SCAN_NONDEEP_WRITE);

    rv = write_scan_chunk (
        ctxt,
        part_index,
        pctxt,
        part,
        y,
        packed_data,
        packed_size,
        unpacked_size,
        sample_data,
        sample_data_size);
    return EXR_UNLOCK (pctxt), rv;
}

/**************************************/

/* pull most of the logic to here to avoid having to unlock at every
 * error exit point and re-use mostly shared logic */
static exr_result_t
write_tile_chunk (
    exr_context_t                 ctxt,
    int                           part_index,
    struct _internal_exr_context* pctxt,
    struct _internal_exr_part*    part,
    int                           tilex,
    int                           tiley,
    int                           levelx,
    int                           levely,
    const void*                   packed_data,
    size_t                        packed_size,
    size_t                        unpacked_size,
    const void*                   sample_data,
    size_t                        sample_data_size)
{
    exr_result_t rv;
    int32_t      data[6];
    int32_t      psize;
    int          cidx, wrcnt;
    uint64_t*    ctable;

    int                        tilew, tileh = 0;
    const exr_attr_tiledesc_t* tiledesc;

    if (pctxt->mode != EXR_CONTEXT_WRITING_DATA)
    {
        if (pctxt->mode == EXR_CONTEXT_WRITE)
            return pctxt->standard_error (ctxt, EXR_ERR_HEADER_NOT_WRITTEN);
        return pctxt->standard_error (ctxt, EXR_ERR_NOT_OPEN_WRITE);
    }

    if (part->storage_mode == EXR_STORAGE_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_SCANLINE)
    {
        return pctxt->standard_error (ctxt, EXR_ERR_TILE_SCAN_MIXEDAPI);
    }

    if (pctxt->cur_output_part != part_index)
        return pctxt->standard_error (ctxt, EXR_ERR_PART_NOT_READY);

    if (!packed_data || packed_size == 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid packed data argument size %" PRIu64 " pointer %p",
            (uint64_t) packed_size,
            packed_data);

    if (part->storage_mode != EXR_STORAGE_DEEP_TILED &&
        packed_size > (size_t) INT32_MAX)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Packed data size %" PRIu64 " too large (max %" PRIu64 ")",
            (uint64_t) packed_size,
            (uint64_t) INT32_MAX);
    psize = (int32_t) packed_size;

    if (part->storage_mode == EXR_STORAGE_DEEP_TILED &&
        (!sample_data || sample_data_size == 0))
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid sample count data argument size %" PRIu64 " pointer %p",
            (uint64_t) sample_data_size,
            sample_data);

    if (!part->tiles || part->num_tile_levels_x <= 0 ||
        part->num_tile_levels_y <= 0 || !part->tile_level_tile_count_x ||
        !part->tile_level_tile_count_y)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_BAD_CHUNK_DATA,
            "Attempting to write tiled data, missing tile description");
    }

    cidx = -1;
    rv   = compute_tile_chunk_off (
        pctxt, part, tilex, tiley, levelx, levely, &cidx);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (cidx < 0 || cidx >= part->chunk_count)
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Chunk index for tile (%d, %d) at level (%d, %d) %d outside chunk count %d",
            tilex,
            tiley,
            levelx,
            levely,
            cidx,
            part->chunk_count);
    }

    if (part->lineorder != EXR_LINEORDER_RANDOM_Y &&
        pctxt->last_output_chunk != (cidx - 1))
    {
        return pctxt->print_error (
            ctxt,
            EXR_ERR_CHUNK_NOT_READY,
            "Chunk index %d is not the next chunk to be written (last %d)",
            cidx,
            pctxt->last_output_chunk);
    }

    wrcnt = 0;
    if (pctxt->is_multipart) { data[wrcnt++] = part_index; }
    data[wrcnt++] = tilex;
    data[wrcnt++] = tiley;
    data[wrcnt++] = levelx;
    data[wrcnt++] = levely;
    if (part->storage_mode != EXR_STORAGE_DEEP_TILED) data[wrcnt++] = psize;

    priv_from_native32 (data, wrcnt);

    rv = alloc_chunk_table (pctxt, part, &ctable);
    if (rv != EXR_ERR_SUCCESS) return rv;

    ctable[cidx] = pctxt->output_file_offset;
    rv           = pctxt->do_write (
        pctxt, data, wrcnt * sizeof (int32_t), &(pctxt->output_file_offset));
    if (rv == EXR_ERR_SUCCESS && part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        int64_t ddata[3];
        ddata[0] = (uint64_t) sample_data_size;
        ddata[1] = (uint64_t) packed_size;
        ddata[2] = (uint64_t) unpacked_size;
        rv       = pctxt->do_write (
            pctxt, ddata, 3 * sizeof (uint64_t), &(pctxt->output_file_offset));

        if (rv == EXR_ERR_SUCCESS)
            rv = pctxt->do_write (
                pctxt,
                sample_data,
                sample_data_size,
                &(pctxt->output_file_offset));
    }
    if (rv == EXR_ERR_SUCCESS)
        rv = pctxt->do_write (
            pctxt, packed_data, packed_size, &(pctxt->output_file_offset));

    if (rv == EXR_ERR_SUCCESS)
    {
        ++(pctxt->output_chunk_count);
        if (pctxt->output_chunk_count == part->chunk_count)
        {
            uint64_t chunkoff = part->chunk_table_offset;

            ++(pctxt->cur_output_part);
            if (pctxt->cur_output_part == pctxt->num_parts)
                pctxt->mode = EXR_CONTEXT_WRITE_FINISHED;
            pctxt->last_output_chunk  = -1;
            pctxt->output_chunk_count = 0;

            priv_from_native64 (ctable, part->chunk_count);
            rv = pctxt->do_write (
                pctxt,
                ctable,
                sizeof (uint64_t) * part->chunk_count,
                &chunkoff);
            /* just in case we look at it again? */
            priv_to_native64 (ctable, part->chunk_count);
        }
        else
        {
            pctxt->last_output_chunk = cidx;
        }
    }

    return rv;
}

/**************************************/

exr_result_t
exr_write_tile_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           tilex,
    int           tiley,
    int           levelx,
    int           levely,
    const void*   packed_data,
    size_t        packed_size)
{
    exr_result_t rv;
    EXR_PROMOTE_LOCKED_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_DEEP_TILED)
        return EXR_UNLOCK (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_USE_TILE_DEEP_WRITE);

    rv = write_tile_chunk (
        ctxt,
        part_index,
        pctxt,
        part,
        tilex,
        tiley,
        levelx,
        levely,
        packed_data,
        packed_size,
        0,
        NULL,
        0);
    return EXR_UNLOCK (pctxt), rv;
}

/**************************************/

exr_result_t
exr_write_deep_tile_chunk (
    exr_context_t ctxt,
    int           part_index,
    int           tilex,
    int           tiley,
    int           levelx,
    int           levely,
    const void*   packed_data,
    size_t        packed_size,
    size_t        unpacked_size,
    const void*   sample_data,
    size_t        sample_data_size)
{
    exr_result_t rv;
    EXR_PROMOTE_LOCKED_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (part->storage_mode == EXR_STORAGE_TILED)
        return EXR_UNLOCK (pctxt),
               pctxt->standard_error (ctxt, EXR_ERR_USE_TILE_NONDEEP_WRITE);

    rv = write_tile_chunk (
        ctxt,
        part_index,
        pctxt,
        part,
        tilex,
        tiley,
        levelx,
        levely,
        packed_data,
        packed_size,
        unpacked_size,
        sample_data,
        sample_data_size);
    return EXR_UNLOCK (pctxt), rv;
}
