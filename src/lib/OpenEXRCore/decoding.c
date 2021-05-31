/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_decode.h"

#include "internal_decompress.h"
#include "internal_structs.h"
#include "internal_unpack.h"
#include "internal_xdr.h"

#include <stdio.h>
#include <string.h>

/**************************************/

static void
free_buffer (
    const struct _internal_exr_context* pctxt,
    exr_decode_pipeline_t*              decode,
    enum transcoding_pipeline_buffer_id bufid,
    void**                              buf,
    size_t*                             sz)
{
    void*  curbuf = *buf;
    size_t cursz  = *sz;
    if (curbuf && cursz > 0)
    {
        if (decode->free_fn)
            decode->free_fn (bufid, curbuf);
        else
            pctxt->free_fn (curbuf);
        *buf = NULL;
        *sz  = 0;
    }
}

/**************************************/

static exr_result_t
alloc_buffer (
    const struct _internal_exr_context* pctxt,
    exr_decode_pipeline_t*              decode,
    enum transcoding_pipeline_buffer_id bufid,
    void**                              buf,
    size_t*                             cursz,
    size_t                              newsz)
{
    void* curbuf = *buf;
    if (!curbuf || *cursz < newsz)
    {
        free_buffer (pctxt, decode, bufid, buf, cursz);

        if (decode->alloc_fn)
            curbuf = decode->alloc_fn (bufid, newsz);
        else
            curbuf = pctxt->alloc_fn (newsz);
        if (curbuf == NULL)
            return pctxt->print_error (
                pctxt,
                EXR_ERR_OUT_OF_MEMORY,
                "Unable to allocate %" PRIu64 " bytes",
                (uint64_t) newsz);
        *buf   = curbuf;
        *cursz = newsz;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_result_t
update_pack_unpack_ptrs (
    const struct _internal_exr_context* pctxt, exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    if (decode->chunk_block.packed_size == decode->chunk_block.unpacked_size)
    {
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size));

        decode->unpacked_alloc_size = 0;
        decode->unpacked_buffer     = decode->packed_buffer;
        rv                          = EXR_ERR_SUCCESS;
    }
    else
    {
        rv = alloc_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size),
            decode->chunk_block.unpacked_size);
    }
    return rv;
}

static exr_result_t
read_uncompressed_direct (exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    int          height, start_y;
    uint64_t     dataoffset, toread;
    uint8_t*     cdata;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (
        decode->context, decode->part_index);

    dataoffset = decode->chunk_block.data_offset;

    height  = decode->chunk_block.height;
    start_y = decode->chunk_block.start_y;
    for (int y = 0; y < height; ++y)
    {
        for (int c = 0; c < decode->channel_count; ++c)
        {
            exr_coding_channel_info_t* decc = (decode->channels + c);

            cdata = decc->decode_to_ptr;
            toread =
                (uint64_t) decc->width * (uint64_t) decc->bytes_per_element;

            if (decc->height == 0) continue;

            if (decc->y_samples > 1)
            {
                cdata +=
                    ((uint64_t) (y / decc->y_samples) *
                     (uint64_t) decc->output_line_stride);
            }
            else
            {
                cdata += (uint64_t) y * (uint64_t) decc->output_line_stride;
            }

            /* actual read into the output pointer */
            rv = pctxt->do_read (
                pctxt, cdata, toread, &dataoffset, NULL, EXR_MUST_READ_ALL);
            if (rv != EXR_ERR_SUCCESS) return rv;

            // need to swab them to native
            if (decc->bytes_per_element == 2)
                priv_to_native16 (cdata, decc->width);
            else
                priv_to_native32 (cdata, decc->width);
        }
    }

    return EXR_ERR_SUCCESS;
}

static exr_result_t
default_read_chunk (exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (
        decode->context, decode->part_index);

    rv = alloc_buffer (
        pctxt,
        decode,
        EXR_TRANSCODE_BUFFER_PACKED,
        &(decode->packed_buffer),
        &(decode->packed_alloc_size),
        decode->chunk_block.packed_size);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        rv = alloc_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(decode->packed_sample_count_table),
            &(decode->packed_sample_count_alloc_size),
            decode->chunk_block.sample_count_table_size);
        if (rv != EXR_ERR_SUCCESS) return rv;
        rv = exr_read_deep_chunk (
            decode->context,
            decode->part_index,
            &(decode->chunk_block),
            decode->packed_buffer,
            decode->packed_sample_count_table);
    }
    else
    {
        rv = exr_read_chunk (
            decode->context,
            decode->part_index,
            &(decode->chunk_block),
            decode->packed_buffer);
    }

    return rv;
}

static exr_result_t
decompress_data (
    const struct _internal_exr_context* pctxt,
    const exr_compression_t             ctype,
    exr_decode_pipeline_t*              decode,
    void*                               packbufptr,
    size_t                              packsz,
    void*                               unpackbufptr,
    size_t                              unpacksz)
{
    exr_result_t rv;

    switch (ctype)
    {
        case EXR_COMPRESSION_NONE:
            return pctxt->report_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "no compresssion set but still trying to decompress");

        case EXR_COMPRESSION_RLE:
            rv = internal_exr_undo_rle (
                decode, packbufptr, packsz, unpackbufptr, unpacksz);
            break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS:
            rv = alloc_buffer (
                pctxt,
                decode,
                EXR_TRANSCODE_BUFFER_SCRATCH1,
                &(decode->scratch_buffer_1),
                &(decode->scratch_alloc_size_1),
                unpacksz);
            if (rv != EXR_ERR_SUCCESS) return rv;

            rv = internal_exr_undo_zip (
                decode,
                packbufptr,
                packsz,
                unpackbufptr,
                unpacksz,
                decode->scratch_buffer_1,
                decode->scratch_alloc_size_1);
            break;
        case EXR_COMPRESSION_PIZ:
        case EXR_COMPRESSION_PXR24:
        case EXR_COMPRESSION_B44:
        case EXR_COMPRESSION_B44A:
        case EXR_COMPRESSION_DWAA:
        case EXR_COMPRESSION_DWAB:
            return pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X not yet implemented",
                ctype);
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            return pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X invalid",
                ctype);
    }
    return rv;
}

static exr_result_t
default_decompress_chunk (exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (
        decode->context, decode->part_index);

    rv = update_pack_unpack_ptrs (pctxt, decode);
    if (rv != EXR_ERR_SUCCESS) return rv;

    if (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
        part->storage_mode == EXR_STORAGE_DEEP_TILED)
    {
        size_t unpack_sample_size =
            (size_t) (decode->chunk_block.width) * sizeof (int32_t);

        rv = alloc_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_SAMPLES,
            (void**) &(decode->sample_count_table),
            &(decode->sample_count_alloc_size),
            unpack_sample_size);

        if (rv != EXR_ERR_SUCCESS) return rv;

        if (decode->chunk_block.sample_count_table_size == unpack_sample_size)
            memcpy (
                decode->sample_count_table,
                decode->packed_sample_count_table,
                unpack_sample_size);
        else
            rv = decompress_data (
                pctxt,
                part->comp_type,
                decode,
                decode->packed_sample_count_table,
                decode->chunk_block.sample_count_table_size,
                decode->sample_count_table,
                unpack_sample_size);
    }

    if (decode->unpacked_buffer != decode->packed_buffer)
    {
        rv = decompress_data (
            pctxt,
            part->comp_type,
            decode,
            decode->packed_buffer,
            decode->chunk_block.packed_size,
            decode->unpacked_buffer,
            decode->chunk_block.unpacked_size);
    }

    return rv;
}

/**************************************/

exr_result_t
exr_initialize_decoding (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_decode_pipeline_t*        decode)
{
    int                        chans;
    exr_attr_chlist_t*         chanlist;
    exr_coding_channel_info_t* chandecodes;
    exr_decode_pipeline_t      nil = { 0 };

    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!cinfo || !decode)
        return pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT);

    *decode = nil;

    chanlist = part->channels->chlist;
    chans    = chanlist->num_channels;
    if (chans <= 5) { chandecodes = decode->_quick_chan_store; }
    else
    {
        chandecodes = pctxt->alloc_fn (
            (size_t) (chans) * sizeof (exr_coding_channel_info_t));
        if (chandecodes == NULL)
            return pctxt->standard_error (pctxt, EXR_ERR_OUT_OF_MEMORY);
        memset (
            chandecodes,
            0,
            (size_t) (chans) * sizeof (exr_coding_channel_info_t));
    }

    decode->channels      = chandecodes;
    decode->channel_count = (int16_t) chans;
    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     decc = (chandecodes + c);

        decc->channel_name = curc->name.str;

        if (curc->y_sampling > 1)
        {
            if (cinfo->height == 1)
                decc->height = ((cinfo->start_y % curc->y_sampling) == 0) ? 1
                                                                          : 0;
            else
                decc->height = cinfo->height / curc->y_sampling;
        }
        else
            decc->height = cinfo->height;

        if (curc->x_sampling > 1)
            decc->width = cinfo->width / curc->x_sampling;
        else
            decc->width = cinfo->width;

        decc->x_samples         = curc->x_sampling;
        decc->y_samples         = curc->y_sampling;
        decc->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        decc->data_type         = (uint16_t) (curc->pixel_type);

        /* initialize these so they don't trip us up during decoding
         * when the user also chooses to skip a channel */
        decc->output_bytes_per_element = decc->bytes_per_element;
        decc->output_data_type         = decc->data_type;
        /* but leave the rest as zero for the user to fill in */
    }

    decode->context     = ctxt;
    decode->part_index  = part_index;
    decode->chunk_block = *cinfo;
    return EXR_ERR_SUCCESS;
}

exr_result_t
exr_decoding_choose_default_routines (
    exr_const_context_t ctxt, int part_index, exr_decode_pipeline_t* decode)
{
    int32_t isdeep = 0, chanstofill = 0, chanstounpack = 0, sametype = -2,
            sameouttype = -2, samebpc = 0, sameoutbpc = 0, hassampling = 0,
            hastypechange = 0, simpinterleave = 0, simplineoff = 0,
            sameoutinc     = 0;
    uint8_t* interleaveptr = NULL;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!decode) return pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT);

    if (decode->context != ctxt || decode->part_index != part_index)
        return pctxt->print_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part");

    isdeep = (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
              part->storage_mode == EXR_STORAGE_DEEP_TILED)
                 ? 1
                 : 0;

    for (int c = 0; c < decode->channel_count; ++c)
    {
        exr_coding_channel_info_t* decc = (decode->channels + c);

        if (decc->height == 0 || !decc->decode_to_ptr) continue;

        /*
         * if a user specifies a bad pixel stride / line stride
         * we can't know this realistically, and they may want to
         * use 0 to cause things to collapse for testing purposes
         * so only test the values we know we use for decisions
         */
        if (decc->output_bytes_per_element != 2 &&
            decc->output_bytes_per_element != 4)
            return pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output bytes per element (%d) for channel %c (%s)",
                (int) decc->output_bytes_per_element,
                c,
                decc->channel_name);

        if (decc->output_data_type != (uint16_t)(EXR_PIXEL_HALF) &&
            decc->output_data_type != (uint16_t)(EXR_PIXEL_FLOAT) &&
            decc->output_data_type != (uint16_t)(EXR_PIXEL_UINT) )
            return pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output data type (%d) for channel %c (%s)",
                (int) decc->output_data_type,
                c,
                decc->channel_name);

        if (sametype == -2)
            sametype = (int32_t) decc->data_type;
        else if (sametype != (int32_t) decc->data_type)
            sametype = -1;

        if (sameouttype == -2)
            sameouttype = (int32_t) decc->output_data_type;
        else if (sameouttype != (int32_t) decc->output_data_type)
            sameouttype = -1;

        if (samebpc == 0)
            samebpc = decc->bytes_per_element;
        else if (samebpc != decc->bytes_per_element)
            samebpc = -1;

        if (sameoutbpc == 0)
            sameoutbpc = decc->output_bytes_per_element;
        else if (sameoutbpc != decc->output_bytes_per_element)
            sameoutbpc = -1;

        if (decc->x_samples != 1 || decc->y_samples != 1) hassampling = 1;

        ++chanstofill;
        if (decc->output_pixel_stride != decc->bytes_per_element)
            ++chanstounpack;
        if (decc->output_data_type != decc->data_type) ++hastypechange;

        if (simplineoff == 0)
            simplineoff = decc->output_line_stride;
        else if (simplineoff != decc->output_line_stride)
            simplineoff = -1;

        if (simpinterleave == 0)
        {
            interleaveptr  = decc->decode_to_ptr;
            simpinterleave = decc->output_pixel_stride;
        }
        else if (
            simpinterleave != decc->output_pixel_stride ||
            decc->decode_to_ptr !=
                (interleaveptr + c * decc->output_bytes_per_element))
        {
            interleaveptr  = NULL;
            simpinterleave = -1;
        }

        if (sameoutinc == 0)
            sameoutinc = decc->output_pixel_stride;
        else if (sameoutinc != decc->output_pixel_stride)
            sameoutinc = -1;
    }

    if (simpinterleave != sameoutbpc * decode->channel_count ||
        interleaveptr == NULL)
        simpinterleave = -1;

    /* special case, uncompressed and reading planar data straight in
     * to all the channels */
    if (!isdeep &&
        decode->chunk_block.packed_size == decode->chunk_block.unpacked_size &&
        chanstounpack == 0 && hastypechange == 0 && chanstofill > 0 &&
        chanstofill == decode->channel_count)
    {
        decode->read_fn               = &read_uncompressed_direct;
        decode->decompress_fn         = NULL;
        decode->unpack_and_convert_fn = NULL;
        return EXR_ERR_SUCCESS;
    }
    decode->read_fn = &default_read_chunk;
    if (part->comp_type != EXR_COMPRESSION_NONE)
        decode->decompress_fn = &default_decompress_chunk;

    decode->unpack_and_convert_fn = internal_exr_match_decode (
        decode,
        isdeep,
        chanstofill,
        chanstounpack,
        sametype,
        sameouttype,
        samebpc,
        sameoutbpc,
        hassampling,
        hastypechange,
        sameoutinc,
        simpinterleave,
        simplineoff);

    if (!decode->unpack_and_convert_fn)
        return pctxt->report_error (
            pctxt,
            EXR_ERR_ARGUMENT_OUT_OF_RANGE,
            "Unable to choose valid unpack routine");

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_decoding_update (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_decode_pipeline_t*        decode)
{
    int                        chans;
    exr_attr_chlist_t*         chanlist;
    exr_coding_channel_info_t* chandecodes;

    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!cinfo || !decode)
        return pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT);

    if (decode->context != ctxt || decode->part_index != part_index)
        return pctxt->report_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for decoding update from different context / part");

    chanlist    = part->channels->chlist;
    chans       = chanlist->num_channels;
    chandecodes = decode->channels;

    if (decode->unpacked_buffer == decode->packed_buffer)
        decode->unpacked_buffer = NULL;

    if (decode->channel_count != chanlist->num_channels)
        return pctxt->report_error (
            pctxt, EXR_ERR_INVALID_ARGUMENT, "Mismatch in channel counts");

    for (int c = 0; c < chans; ++c)
    {
        const exr_attr_chlist_entry_t* curc = (chanlist->entries + c);
        exr_coding_channel_info_t*     decc = (chandecodes + c);

        decc->channel_name = curc->name.str;

        if (curc->y_sampling > 1)
        {
            if (cinfo->height == 1)
                decc->height = ((cinfo->start_y % curc->y_sampling) == 0) ? 1
                                                                          : 0;
            else
                decc->height = cinfo->height / curc->y_sampling;
        }
        else
            decc->height = cinfo->height;

        if (curc->x_sampling > 1)
            decc->width = cinfo->width / curc->x_sampling;
        else
            decc->width = cinfo->width;
        decc->x_samples = curc->x_sampling;
        decc->y_samples = curc->y_sampling;

        decc->bytes_per_element = (curc->pixel_type == EXR_PIXEL_HALF) ? 2 : 4;
        decc->data_type         = (uint16_t) (curc->pixel_type);
    }

    decode->chunk_block = *cinfo;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_decoding_run (
    exr_const_context_t ctxt, int part_index, exr_decode_pipeline_t* decode)
{
    exr_result_t rv;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!decode) return pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT);
    if (decode->context != ctxt || decode->part_index != part_index)
        return pctxt->report_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for decoding update from different context / part");

    if (!decode->read_fn)
        return pctxt->report_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Decode pipeline has no read_fn declared");
    rv = decode->read_fn (decode);
    if (rv == EXR_ERR_SUCCESS)
    {
        if (decode->decompress_fn)
            rv = decode->decompress_fn (decode);
        else
        {
            free_buffer (
                pctxt,
                decode,
                EXR_TRANSCODE_BUFFER_UNPACKED,
                &(decode->unpacked_buffer),
                &(decode->unpacked_alloc_size));

            if (decode->chunk_block.packed_size ==
                decode->chunk_block.unpacked_size)
            {
                decode->unpacked_buffer = decode->packed_buffer;
            }
        }
    }

    if (rv == EXR_ERR_SUCCESS && decode->unpack_and_convert_fn)
        rv = decode->unpack_and_convert_fn (decode);

    return rv;
}

/**************************************/

exr_result_t
exr_destroy_decoding (exr_const_context_t ctxt, exr_decode_pipeline_t* decode)
{
    INTERN_EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);
    if (decode)
    {
        exr_decode_pipeline_t nil = { 0 };
        if (decode->channels != decode->_quick_chan_store)
            pctxt->free_fn (decode->channels);

        if (decode->unpacked_buffer == decode->packed_buffer &&
            decode->unpacked_alloc_size == 0)
            decode->unpacked_buffer = NULL;

        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(decode->packed_buffer),
            &(decode->packed_alloc_size));
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_UNPACKED,
            &(decode->unpacked_buffer),
            &(decode->unpacked_alloc_size));
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(decode->scratch_buffer_1),
            &(decode->scratch_alloc_size_1));
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_SCRATCH2,
            &(decode->scratch_buffer_2),
            &(decode->scratch_alloc_size_2));
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(decode->packed_sample_count_table),
            &(decode->packed_sample_count_alloc_size));
        free_buffer (
            pctxt,
            decode,
            EXR_TRANSCODE_BUFFER_SAMPLES,
            (void**) &(decode->sample_count_table),
            &(decode->sample_count_alloc_size));
        *decode = nil;
    }
    return EXR_ERR_SUCCESS;
}
