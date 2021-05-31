/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_UNPACK_H
#define OPENEXR_CORE_UNPACK_H

#include "openexr_decode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef exr_result_t (*internal_exr_unpack_fn) (exr_decode_pipeline_t*);

internal_exr_unpack_fn internal_exr_match_decode (
    exr_decode_pipeline_t* decode,
    int                    isdeep,
    int                    chanstofill,
    int                    chanstounpack,
    int                    sametype,
    int                    sameouttype,
    int                    samebpc,
    int                    sameoutbpc,
    int                    hassampling,
    int                    hastypechange,
    int                    sameoutinc,
    int                    simpinterleave,
    int                    simplineoff);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_UNPACK_H */
