// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#include "openexr.h"

#include "internal_constants.h"
#include "internal_file.h"

#include <string.h>

/**************************************/

exr_result_t exr_encode_chunk_init_scanline (
    const exr_context_t      ctxt,
    int                      part_index,
    exr_encode_chunk_info_t* outinfo,
    int                      y,
    int                      own_scratch_space)
{
    return EXR_ERR_UNKNOWN;
}

/**************************************/

exr_result_t exr_encode_chunk_init_tile (
    const exr_context_t      ctxt,
    int                      part_index,
    exr_encode_chunk_info_t* outinfo,
    int                      tilex,
    int                      tiley,
    int                      levelx,
    int                      levely,
    int                      own_scratch_space)
{
    return EXR_ERR_UNKNOWN;
}


exr_result_t
exr_write_chunk (exr_context_t ctxt, exr_encode_chunk_info_t* cinfo)
{
    return EXR_ERR_UNKNOWN;
}

exr_result_t exr_compress_data (
    const exr_context_t     ctxt,
    const exr_compression_t ctype,
    void*                   compressed_data,
    size_t                  comp_buf_size,
    const void*             uncompressed_data,
    size_t                  uncompressed_size)
{
    return EXR_ERR_UNKNOWN;
}
