/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#define _LARGEFILE64_SOURCE

#include "openexr_context.h"

#include "openexr_priv_constants.h"
#include "openexr_priv_file.h"
#include "openexr_priv_memory.h"

#include <IlmThreadConfig.h>

#ifdef ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
#        include <synchapi.h>
#    else
#        include <pthread.h>
#    endif
#endif

#if defined(_WIN32) || defined(_WIN64)
#    include "openexr_file_win32_impl.h"
#else
#    include "openexr_file_posix_impl.h"
#endif

/**************************************/

static exr_result_t
dispatch_read (
    const struct _internal_exr_context* ctxt,
    void*                               buf,
    uint64_t                            sz,
    uint64_t*                           offsetp,
    int64_t*                            nread,
    enum _INTERNAL_EXR_READ_MODE        rmode)
{
    int64_t      rval = -1;
    exr_result_t rv   = EXR_ERR_UNKNOWN;

    if (nread) *nread = rval;

    if (!ctxt) return EXR_ERR_INVALID_ARGUMENT;

    if (!offsetp)
        return ctxt->report_error (
            (const exr_context_t) ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "read requested with no output offset pointer");

    if (ctxt->read_fn)
        rval = ctxt->read_fn (
            (const exr_context_t) ctxt,
            ctxt->user_data,
            buf,
            sz,
            *offsetp,
            ctxt->print_error);
    else
        return ctxt->standard_error (
            (const exr_context_t) ctxt, EXR_ERR_NOT_OPEN_READ);

    if (nread) *nread = rval;
    if (rval > 0) *offsetp += rval;

    if (rval == (int64_t) sz || (rmode == EXR_ALLOW_SHORT_READ && rval >= 0))
        rv = EXR_ERR_SUCCESS;
    else
        rv = EXR_ERR_READ_IO;
    return rv;
}

/**************************************/

static exr_result_t
dispatch_write (
    struct _internal_exr_context* ctxt,
    const void*                   buf,
    uint64_t                      sz,
    uint64_t*                     offsetp)
{
    int64_t      rval = -1;
    exr_result_t rv   = EXR_ERR_UNKNOWN;
    uint64_t     outpos;

    if (!ctxt) return EXR_ERR_INVALID_ARGUMENT;

    if (!offsetp)
        return ctxt->report_error (
            (const exr_context_t) ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "write requested with no output offset pointer");

    outpos = (uint64_t) atomic_fetch_add (&(ctxt->file_offset), (uint64_t) sz);
    if (ctxt->write_fn)
        rval = ctxt->write_fn (
            (const exr_context_t) ctxt,
            ctxt->user_data,
            buf,
            sz,
            outpos,
            ctxt->print_error);
    else
        return ctxt->standard_error (
            (const exr_context_t) ctxt, EXR_ERR_NOT_OPEN_WRITE);

    if (rval > 0)
        *offsetp = outpos + rval;
    else
        *offsetp = outpos;

    return (rval == (int64_t) sz) ? EXR_ERR_SUCCESS : EXR_ERR_WRITE_IO;
}

/**************************************/

static exr_result_t
process_query_size (
    struct _internal_exr_context* ctxt, exr_context_initializer_t* inits)
{
    if (inits->size_fn)
    {
        ctxt->file_size =
            (inits->size_fn) ((const exr_context_t) ctxt, ctxt->user_data);
    }
    else
    {
        ctxt->file_size = -1;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_finish (exr_context_t* pctxt)
{
    struct _internal_exr_context* ctxt;
    int                           rv = 0;

    if (!pctxt) return EXR_ERR_INVALID_ARGUMENT;

    ctxt = EXR_CTXT (*pctxt);
    if (ctxt)
    {
        int failed = 0;
        if (ctxt->mode == EXR_CONTEXT_WRITE)
        {
            printf ("TODO: check all chunks written and write chunk offsets\n");
            failed = 1;

            rv = finalize_write (ctxt, failed);
        }

        if (ctxt->destroy_fn)
            ctxt->destroy_fn (*pctxt, ctxt->user_data, failed);

        internal_exr_destroy_context (ctxt);
    }
    *pctxt = NULL;

    return rv;
}

/**************************************/

exr_result_t
exr_start_read (
    exr_context_t*                   ctxt,
    const char*                      filename,
    const exr_context_initializer_t* initdata)
{
    exr_result_t                  rv    = EXR_ERR_UNKNOWN;
    struct _internal_exr_context* ret   = NULL;
    exr_context_initializer_t     inits = EXR_DEFAULT_CONTEXT_INITIALIZER;

    if (initdata) inits = *initdata;

    internal_exr_update_default_handlers (&inits);

    if (!ctxt)
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid context handle passed to start_read function");
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if (filename && filename[0] != '\0')
    {
        rv = internal_exr_alloc_context (
            &ret,
            &inits,
            EXR_CONTEXT_READ,
            sizeof (struct _internal_exr_filehandle));
        if (rv == EXR_ERR_SUCCESS)
        {
            ret->do_read = &dispatch_read;

            rv = exr_attr_string_create (ret, &(ret->filename), filename);
            if (rv == EXR_ERR_SUCCESS)
            {
                if (!inits.read_fn)
                {
                    inits.size_fn = &default_query_size_func;
                    rv            = default_init_read_file (ret);
                }

                if (rv == EXR_ERR_SUCCESS)
                    rv = process_query_size (ret, &inits);
                if (rv == EXR_ERR_SUCCESS) rv = internal_exr_parse_header (ret);
            }

            if (rv != EXR_ERR_SUCCESS) exr_finish ((exr_context_t*) &ret);
        }
        else
            rv = EXR_ERR_OUT_OF_MEMORY;
    }
    else
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid filename passed to start_read function");
        rv = EXR_ERR_INVALID_ARGUMENT;
    }

    *ctxt = (exr_context_t) ret;
    return rv;
}

/**************************************/

exr_result_t
exr_start_write (
    exr_context_t*                   ctxt,
    const char*                      filename,
    enum exr_default_write_mode      default_mode,
    const exr_context_initializer_t* initdata)
{
    int                           rv    = EXR_ERR_UNKNOWN;
    struct _internal_exr_context* ret   = NULL;
    exr_context_initializer_t     inits = EXR_DEFAULT_CONTEXT_INITIALIZER;

    if (initdata) inits = *initdata;

    internal_exr_update_default_handlers (&inits);

    if (!ctxt)
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid context handle passed to start_read function");
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if (filename && filename[0] != '\0')
    {
        rv = internal_exr_alloc_context (
            &ret,
            &inits,
            EXR_CONTEXT_WRITE,
            sizeof (struct _internal_exr_filehandle));
        if (rv == EXR_ERR_SUCCESS)
        {
            ret->do_write = &dispatch_write;

            rv = exr_attr_string_create (ret, &(ret->filename), filename);

            if (rv == EXR_ERR_SUCCESS)
            {
                if (!inits.write_fn)
                {
                    if (default_mode == EXR_INTERMEDIATE_TEMP_FILE)
                        rv = make_temp_filename (ret);
                    if (rv == EXR_ERR_SUCCESS)
                        rv = default_init_write_file (ret);
                }
            }

            if (rv != EXR_ERR_SUCCESS) exr_finish ((exr_context_t*) &ret);
        }
        else
            rv = EXR_ERR_OUT_OF_MEMORY;
    }
    else
    {
        inits.error_handler_fn (
            NULL,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid filename passed to start_write function");
        rv = EXR_ERR_INVALID_ARGUMENT;
    }

    *ctxt = (exr_context_t) ret;
    return rv;
}

/**************************************/

exr_result_t
exr_start_inplace_header_update (
    exr_context_t*                   ctxt,
    const char*                      filename,
    const exr_context_initializer_t* ctxtdata)
{
    /* TODO: not yet implemented */
    return EXR_ERR_INVALID_ARGUMENT;
}

/**************************************/

exr_result_t
exr_get_file_name (const exr_context_t ctxt, const char** name)
{
    EXR_PROMOTE_CONST_CONTEXT_OR_ERROR (ctxt);

    if (name)
    {
        *name = pctxt->filename.str;
        return EXR_ERR_SUCCESS;
    }

    return pctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);
}
