/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_encode.h"

#include "internal_coding.h"
#include "internal_compress.h"
#include "internal_structs.h"
#include "internal_xdr.h"

/**************************************/

static void
free_buffer (
    const struct _internal_exr_context* pctxt,
    exr_encode_pipeline_t*              encode,
    enum transcoding_pipeline_buffer_id bufid,
    void**                              buf,
    size_t*                             sz)
{
    void*  curbuf = *buf;
    size_t cursz  = *sz;
    if (curbuf)
    {
        if (cursz > 0)
        {
            if (encode->free_fn)
                encode->free_fn (bufid, curbuf);
            else
                pctxt->free_fn (curbuf);
        }
        *buf = NULL;
        *sz  = 0;
    }
}

/**************************************/

static exr_result_t
alloc_buffer (
    const struct _internal_exr_context* pctxt,
    exr_encode_pipeline_t*              encode,
    enum transcoding_pipeline_buffer_id bufid,
    void**                              buf,
    size_t*                             cursz,
    size_t                              newsz)
{
    void* curbuf = *buf;
    if (!curbuf || *cursz < newsz)
    {
        free_buffer (pctxt, encode, bufid, buf, cursz);

        if (encode->alloc_fn)
            curbuf = encode->alloc_fn (bufid, newsz);
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
default_compress_chunk (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR_NO_LOCK (
        encode->context, encode->part_index);

    rv = alloc_buffer (
        pctxt,
        encode,
        EXR_TRANSCODE_BUFFER_COMPRESSED,
        &(encode->compressed_buffer),
        &(encode->compressed_alloc_size),
        encode->packed_bytes * 2);
    if (rv != EXR_ERR_SUCCESS) return rv;

    switch (part->comp_type)
    {
        case EXR_COMPRESSION_NONE:
            return pctxt->report_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "no compresssion set but still trying to decompress");

        case EXR_COMPRESSION_RLE: rv = internal_exr_apply_rle (encode); break;
        case EXR_COMPRESSION_ZIP:
        case EXR_COMPRESSION_ZIPS:
            rv = alloc_buffer (
                pctxt,
                encode,
                EXR_TRANSCODE_BUFFER_SCRATCH1,
                &(encode->scratch_buffer_1),
                &(encode->scratch_alloc_size_1),
                encode->packed_bytes);
            if (rv != EXR_ERR_SUCCESS) return rv;

            rv = internal_exr_apply_zip (encode);
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
                (int) part->comp_type);
        case EXR_COMPRESSION_LAST_TYPE:
        default:
            return pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Compression technique 0x%02X invalid",
                (int) part->comp_type);
    }
    return rv;
}

/**************************************/

static exr_result_t
default_yield (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (
        encode->context, encode->part_index);

    rv = internal_validate_next_chunk (encode, pctxt, part);

    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (rv);
}

/**************************************/

static exr_result_t
default_write_chunk (exr_encode_pipeline_t* encode)
{
    exr_result_t rv;

    if (!encode) return EXR_ERR_INVALID_ARGUMENT;
    if (!encode->compressed_buffer || encode->compressed_bytes == 0)
        return EXR_ERR_INVALID_ARGUMENT;

    switch (encode->chunk_block.type)
    {
        case EXR_STORAGE_SCANLINE:
            rv = exr_write_scanline_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk_block.start_y,
                encode->compressed_buffer,
                encode->compressed_bytes);
            break;
        case EXR_STORAGE_TILED:
            rv = exr_write_tile_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk_block.start_x,
                encode->chunk_block.start_y,
                encode->chunk_block.level_x,
                encode->chunk_block.level_y,
                encode->compressed_buffer,
                encode->compressed_bytes);
            break;
        case EXR_STORAGE_DEEP_SCANLINE:
            if (!encode->packed_sample_count_table ||
                encode->packed_sample_count_bytes == 0)
                return EXR_ERR_INVALID_ARGUMENT;
            rv = exr_write_deep_scanline_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk_block.start_y,
                encode->compressed_buffer,
                encode->compressed_bytes,
                encode->packed_bytes,
                encode->packed_sample_count_table,
                encode->packed_sample_count_bytes);
            break;
        case EXR_STORAGE_DEEP_TILED:
            if (!encode->packed_sample_count_table ||
                encode->packed_sample_count_bytes == 0)
                return EXR_ERR_INVALID_ARGUMENT;
            rv = exr_write_deep_tile_chunk (
                EXR_CONST_CAST (exr_context_t, encode->context),
                encode->part_index,
                encode->chunk_block.start_x,
                encode->chunk_block.start_y,
                encode->chunk_block.level_x,
                encode->chunk_block.level_y,
                encode->compressed_buffer,
                encode->compressed_bytes,
                encode->packed_bytes,
                encode->packed_sample_count_table,
                encode->packed_sample_count_bytes);
            break;
        case EXR_STORAGE_LAST_TYPE:
        default: rv = EXR_ERR_INVALID_ARGUMENT; break;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_encoding_initialize (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_encode_pipeline_t*        encode)
{
    exr_result_t          rv;
    exr_encode_pipeline_t nil = { 0 };

    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!cinfo || !encode)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
            pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT));

    if (pctxt->mode != EXR_CONTEXT_WRITING_DATA)
    {
        if (pctxt->mode == EXR_CONTEXT_WRITE)
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
                pctxt->standard_error (pctxt, EXR_ERR_HEADER_NOT_WRITTEN));
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
            pctxt->standard_error (pctxt, EXR_ERR_NOT_OPEN_WRITE));
    }

    *encode = nil;

    rv = internal_coding_fill_channel_info (
        &(encode->channels),
        &(encode->channel_count),
        encode->_quick_chan_store,
        cinfo,
        pctxt,
        part);

    if (rv == EXR_ERR_SUCCESS)
    {
        encode->part_index  = part_index;
        encode->context     = ctxt;
        encode->chunk_block = *cinfo;
    }
    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (rv);
}

/**************************************/

exr_result_t
exr_encoding_choose_default_routines (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode)
{
    int32_t isdeep = 0;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!encode)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
            pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT));

    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part"));

    isdeep = (part->storage_mode == EXR_STORAGE_DEEP_SCANLINE ||
              part->storage_mode == EXR_STORAGE_DEEP_TILED)
                 ? 1
                 : 0;

    encode->convert_and_pack_fn = internal_exr_match_encode (encode, isdeep);
    if (part->comp_type != EXR_COMPRESSION_NONE)
        encode->compress_fn = &default_compress_chunk;
    encode->yield_until_ready_fn = &default_yield;
    encode->write_fn             = &default_write_chunk;

    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (EXR_ERR_SUCCESS);
}

/**************************************/

exr_result_t
exr_encoding_update (
    exr_const_context_t           ctxt,
    int                           part_index,
    const exr_chunk_block_info_t* cinfo,
    exr_encode_pipeline_t*        encode)
{
    exr_result_t rv;

    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);
    if (!cinfo || !encode)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
            pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT));

    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Cross-wired request for default routines from different context / part"));

    if (encode->packed_buffer == encode->compressed_buffer)
        encode->compressed_buffer = NULL;

    encode->packed_bytes              = 0;
    encode->packed_sample_count_bytes = 0;
    encode->compressed_bytes          = 0;

    rv = internal_coding_update_channel_info (
        encode->channels, encode->channel_count, cinfo, pctxt, part);

    if (rv == EXR_ERR_SUCCESS) encode->chunk_block = *cinfo;
    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (rv);
}

/**************************************/

exr_result_t
exr_encoding_run (
    exr_const_context_t ctxt, int part_index, exr_encode_pipeline_t* encode)
{
    exr_result_t rv           = EXR_ERR_SUCCESS;
    uint64_t     packed_bytes = 0;
    EXR_PROMOTE_READ_CONST_CONTEXT_AND_PART_OR_ERROR (ctxt, part_index);

    if (!encode)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (
            pctxt->standard_error (pctxt, EXR_ERR_INVALID_ARGUMENT));
    if (encode->context != ctxt || encode->part_index != part_index)
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->report_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid request for encoding update from different context / part"));

    for (int c = 0; c < encode->channel_count; ++c)
    {
        const exr_coding_channel_info_t* encc = (encode->channels + c);

        if (encc->width == 0)
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Unexpected 0-width chunk to encode"));
        if (encc->height == 0)
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Unexpected 0-height chunk to encode"));
        if (!encc->encode_from_ptr)
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Missing channel data pointer - must encode all channels"));

        /*
         * if a user specifies a bad pixel stride / line stride
         * we can't know this realistically, and they may want to
         * use 0 to cause things to collapse for testing purposes
         * so only test the values we can
         */
        if (encc->user_bytes_per_element != 2 &&
            encc->user_bytes_per_element != 4)
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output bytes per element (%d) for channel %c (%s)",
                (int) encc->user_bytes_per_element,
                c,
                encc->channel_name));

        if (encc->user_data_type != (uint16_t) (EXR_PIXEL_HALF) &&
            encc->user_data_type != (uint16_t) (EXR_PIXEL_FLOAT) &&
            encc->user_data_type != (uint16_t) (EXR_PIXEL_UINT))
            return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->print_error (
                pctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid / unsupported output data type (%d) for channel %c (%s)",
                (int) encc->user_data_type,
                c,
                encc->channel_name));

        if (encc->y_samples > 1)
            packed_bytes += (uint64_t) (encc->height / encc->y_samples) *
                            (uint64_t) (encc->width) *
                            (uint64_t) (encc->bytes_per_element);
    }

    if (encode->convert_and_pack_fn)
    {
        rv = alloc_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(encode->packed_buffer),
            &(encode->packed_alloc_size),
            packed_bytes);

        if (rv == EXR_ERR_SUCCESS) rv = encode->convert_and_pack_fn (encode);
    }
    else if (!encode->packed_buffer || encode->packed_bytes == 0)
    {
        return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (pctxt->report_error (
            pctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Encode pipeline has no packing function declared and packed buffer is null / 0 sized"));
    }

    if (rv == EXR_ERR_SUCCESS)
    {
        if (encode->compress_fn) { rv = encode->compress_fn (encode); }
        else
        {
            free_buffer (
                pctxt,
                encode,
                EXR_TRANSCODE_BUFFER_UNPACKED,
                &(encode->compressed_buffer),
                &(encode->compressed_alloc_size));

            encode->compressed_buffer     = encode->packed_buffer;
            encode->compressed_alloc_size = 0;
        }
    }

    if (rv == EXR_ERR_SUCCESS && encode->yield_until_ready_fn)
        rv = encode->yield_until_ready_fn (encode);

    if (rv == EXR_ERR_SUCCESS && encode->write_fn)
        rv = encode->write_fn (encode);

    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (rv);
}

/**************************************/

exr_result_t
exr_encoding_destroy (exr_const_context_t ctxt, exr_encode_pipeline_t* encode)
{
    INTERN_EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);
    if (encode)
    {
        exr_encode_pipeline_t nil = { 0 };
        if (encode->channels != encode->_quick_chan_store)
            pctxt->free_fn (encode->channels);

        free_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_PACKED,
            &(encode->packed_buffer),
            &(encode->packed_alloc_size));
        free_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_COMPRESSED,
            &(encode->compressed_buffer),
            &(encode->compressed_alloc_size));
        free_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH1,
            &(encode->scratch_buffer_1),
            &(encode->scratch_alloc_size_1));
        free_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_SCRATCH2,
            &(encode->scratch_buffer_2),
            &(encode->scratch_alloc_size_2));
        free_buffer (
            pctxt,
            encode,
            EXR_TRANSCODE_BUFFER_PACKED_SAMPLES,
            &(encode->packed_sample_count_table),
            &(encode->packed_sample_count_alloc_size));
        *encode = nil;
    }
    return EXR_UNLOCK_WRITE_AND_RETURN_PCTXT (EXR_ERR_SUCCESS);
}
