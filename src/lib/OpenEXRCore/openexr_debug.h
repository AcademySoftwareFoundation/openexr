/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_DEBUG_H
#define OPENEXR_DEBUG_H

#include "openexr_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Debug function, prints to stdout the parts and attributes of the context passed in */
EXR_EXPORT exr_result_t exr_print_context_info( const exr_context_t c, int verbose );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_DEBUG_H */
