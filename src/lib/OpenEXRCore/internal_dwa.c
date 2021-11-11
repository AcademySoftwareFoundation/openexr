/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_compress.h"
#include "internal_decompress.h"

#include "internal_xdr.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
internal_exr_apply_dwaa (exr_encode_pipeline_t* encode)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR_NO_LOCK (
        encode->context, encode->part_index);
    return pctxt->report_error (
        pctxt,
        EXR_ERR_FEATURE_NOT_IMPLEMENTED,
        "DWA/A compression not yet implemented in C");
}

/**************************************/

exr_result_t
internal_exr_apply_dwab (exr_encode_pipeline_t* encode)
{
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR_NO_LOCK (
        encode->context, encode->part_index);
    return pctxt->report_error (
        pctxt,
        EXR_ERR_FEATURE_NOT_IMPLEMENTED,
        "DWA/B compression not yet implemented in C");
}

exr_result_t
internal_exr_undo_dwaa (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    (void)compressed_data;
    (void)comp_buf_size;
    (void)uncompressed_data;
    (void)uncompressed_size;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR_NO_LOCK (
        decode->context, decode->part_index);
    return pctxt->report_error (
        pctxt,
        EXR_ERR_FEATURE_NOT_IMPLEMENTED,
        "DWA/A decompression not yet implemented in C");
}

exr_result_t
internal_exr_undo_dwab (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    uint64_t               comp_buf_size,
    void*                  uncompressed_data,
    uint64_t               uncompressed_size)
{
    (void)compressed_data;
    (void)comp_buf_size;
    (void)uncompressed_data;
    (void)uncompressed_size;
    EXR_PROMOTE_CONST_CONTEXT_AND_PART_OR_ERROR_NO_LOCK (
        decode->context, decode->part_index);
    return pctxt->report_error (
        pctxt,
        EXR_ERR_FEATURE_NOT_IMPLEMENTED,
        "DWA/B decompression not yet implemented in C");
}
