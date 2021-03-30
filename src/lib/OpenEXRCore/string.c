/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_attr_string_init (exr_context_t ctxt, exr_attr_string_t* s, int32_t len)
{
    exr_attr_string_t nil = { 0 };
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (len < 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized string (%d)",
            len);

    if (!s)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to string object to initialize");

    *s     = nil;
    s->str = (char*) pctxt->alloc_fn (len + 1);
    if (s->str == NULL)
        return pctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
    s->length     = len;
    s->alloc_size = len + 1;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_string_init_static_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* v, int32_t len)
{
    exr_attr_string_t nil = { 0 };
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (len < 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized string (%d)",
            len);

    if (!v)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid static string argument to initialize");

    if (!s)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to string object to initialize");

    *s        = nil;
    s->length = len;
    s->str    = v;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_string_init_static (
    exr_context_t ctxt, exr_attr_string_t* s, const char* v)
{
    size_t  fulllen = 0;
    int32_t length  = 0;
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (v)
    {
        fulllen = strlen (v);
        if (fulllen >= (size_t) INT32_MAX)
            return pctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid string too long for attribute");
        length = (int32_t) fulllen;
    }
    return exr_attr_string_init_static_with_length (ctxt, s, v, length);
}

/**************************************/

exr_result_t
exr_attr_string_create_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* d, int32_t len)
{
    exr_result_t rv;
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (!s)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid (NULL) arguments to string create with length");

    rv = exr_attr_string_init (ctxt, s, len);
    if (rv == EXR_ERR_SUCCESS)
    {
        /* we know we own the string memory */
        char* outs = (char*) s->str;
        /* someone might pass in a string shorter than requested length (or longer) */
        if (len > 0)
        {
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4996)
#endif
            if (d)
                strncpy (outs, d, len);
            else
                memset (outs, 0, len);
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
        }
        outs[len] = '\0';
    }
    return rv;
}

/**************************************/

exr_result_t
exr_attr_string_create (exr_context_t ctxt, exr_attr_string_t* s, const char* d)
{
    int32_t len = 0;
    if (d) len = (int32_t) strlen (d);
    return exr_attr_string_create_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_set_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* d, int32_t len)
{
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (!s)
        return pctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid string argument to string set");

    if (len < 0)
        return pctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to assign a negative sized string (%d)",
            len);

    if (s->alloc_size > len)
    {
        s->length = len;
        /* we own the memory */
        char* sstr = (char*) s->str;
        if (len > 0)
        {
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4996)
#endif
            if (d)
                strncpy (sstr, d, len);
            else
                memset (sstr, 0, len);
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
        }
        sstr[len] = '\0';
        return EXR_ERR_SUCCESS;
    }
    exr_attr_string_destroy (ctxt, s);
    return exr_attr_string_create_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_set (exr_context_t ctxt, exr_attr_string_t* s, const char* d)
{
    int32_t len = 0;
    if (d) len = (int32_t) strlen (d);
    return exr_attr_string_set_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_destroy (exr_context_t ctxt, exr_attr_string_t* s)
{
    EXR_PROMOTE_CONTEXT_OR_ERROR (ctxt);

    if (s)
    {
        exr_attr_string_t nil = { 0 };
        if (s->str && s->alloc_size > 0) pctxt->free_fn ((char*) s->str);
        *s = nil;
    }
    return EXR_ERR_SUCCESS;
}
