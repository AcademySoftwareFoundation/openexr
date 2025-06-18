/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

int
exr_attr_bytes_init (
    exr_context_t ctxt, exr_attr_bytes_t* u, size_t b)
{
    exr_attr_bytes_t nil = {0};

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!u)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to bytes data object to initialize");

    if (b > (size_t) INT32_MAX)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size for bytes data (%" PRIu64
            " bytes, must be <= INT32_MAX)",
            (uint64_t) b);

    *u = nil;
    if (b > 0)
    {
        u->data = (uint8_t*) ctxt->alloc_fn (b);
        if (!u->data)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
    }
    u->size              = (size_t) b;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_bytes_create (
    exr_context_t ctxt, exr_attr_bytes_t* u, size_t b, const void* d)
{
    exr_result_t rv = exr_attr_bytes_init (ctxt, u, b);
    if (rv == EXR_ERR_SUCCESS)
    {
        if (d && u->data) memcpy ((void*) u->data, d, b);
    }

    return rv;
}

/**************************************/

exr_result_t
exr_attr_bytes_destroy (exr_context_t ctxt, exr_attr_bytes_t* ud)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (ud)
    {
        exr_attr_bytes_t nil = {0};
        if (ud->data && ud->size > 0)
            ctxt->free_fn (ud->data);

        *ud = nil;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_bytes_copy (
    exr_context_t           ctxt,
    exr_attr_bytes_t*       ud,
    const exr_attr_bytes_t* srcud)
{
    if (!srcud) return EXR_ERR_INVALID_ARGUMENT;

    return exr_attr_bytes_create (
        ctxt, ud, (size_t) srcud->size, srcud->data);
}
