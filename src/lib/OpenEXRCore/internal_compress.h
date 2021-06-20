/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_COMPRESS_H
#define OPENEXR_CORE_COMPRESS_H

#include "openexr_encode.h"

uint64_t internal_rle_compress (
    void* out, uint64_t outbytes, const void* src, uint64_t srcbytes);

exr_result_t internal_exr_apply_rle (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_zip (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_piz (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_pxr24 (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_b44 (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_b44a (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_dwaa (exr_encode_pipeline_t* encode);

exr_result_t internal_exr_apply_dwab (exr_encode_pipeline_t* encode);

#endif /* OPENEXR_CORE_COMPRESS_H */
