/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_DECOMPRESS_H
#define OPENEXR_CORE_DECOMPRESS_H

#include "openexr_decode.h"

exr_result_t internal_exr_undo_rle (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    size_t                 comp_buf_size,
    void*                  uncompressed_data,
    size_t                 uncompressed_size);

exr_result_t internal_exr_undo_zip (
    exr_decode_pipeline_t* decode,
    const void*            compressed_data,
    size_t                 comp_buf_size,
    void*                  uncompressed_data,
    size_t                 uncompressed_size,
    void*                  scratch_data,
    size_t                 scratch_size);

#endif /* OPENEXR_CORE_DECOMPRESS_H */
