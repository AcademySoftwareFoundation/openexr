/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_structs.h"
#include "internal_attr.h"
#include "internal_constants.h"
#include "internal_memory.h"

#include <IlmThreadConfig.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
#        include <synchapi.h>
#    else
#        include <pthread.h>
#    endif
#endif

/**************************************/

static void
default_error_handler (
    const exr_context_t ctxt, exr_result_t code, const char* msg)
{
    const struct _internal_exr_context* pctxt = EXR_CCTXT (ctxt);

#ifdef ILMBASE_THREADING_ENABLED
#    ifdef _WIN32
    static CRITICAL_SECTION sMutex;
    volatile static long    initialized = 0;
    if (InterlockedIncrement (&initialized) == 1)
        InitializeCriticalSection (&sMutex);
    initialized = 1; // avoids overflow on long running programs...
#    else
    static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
#    endif
#endif

#ifdef ILMBASE_THREADING_ENABLED
#    ifdef _WIN32
    EnterCriticalSection (&sMutex);
#    else
    pthread_mutex_lock (&sMutex);
#    endif
#endif
    if (pctxt)
    {
        if (pctxt->filename.str)
            fprintf (stderr, "%s: %s\n", pctxt->filename.str, msg);
        else
            fprintf (stderr, "File 0x%p: %s\n", (void*) ctxt, msg);
    }
    else
        fprintf (stderr, "<ERROR>: %s\n", msg);
    fflush (stderr);

#ifdef ILMBASE_THREADING_ENABLED
#    ifdef _WIN32
    LeaveCriticalSection (&sMutex);
#    else
    pthread_mutex_unlock (&sMutex);
#    endif
#endif
}

static exr_result_t
dispatch_error (const exr_context_t ctxt, exr_result_t code, const char* msg)
{
    if (ctxt)
    {
        EXR_CCTXT (ctxt)->error_handler_fn (ctxt, code, msg);
        return code;
    }

    default_error_handler (ctxt, code, msg);
    return code;
}

/**************************************/

static exr_result_t
dispatch_standard_error (const exr_context_t ctxt, exr_result_t code)
{
    return dispatch_error (ctxt, code, exr_get_default_error_message (code));
}

/**************************************/

static exr_result_t dispatch_print_error (
    const exr_context_t ctxt, exr_result_t code, const char* msg, ...)
    EXR_PRINTF_FUNC_ATTRIBUTE;

static exr_result_t
dispatch_print_error (
    const exr_context_t ctxt, exr_result_t code, const char* msg, ...)
{
    const struct _internal_exr_context* pctxt = EXR_CCTXT (ctxt);
    char                                stackbuf[256];
    char*                               heapbuf = NULL;
    int                                 nwrit   = 0;
    va_list                             fmtargs;

    va_start (fmtargs, msg);
    {
        va_list stkargs;

        va_copy (stkargs, fmtargs);
        nwrit = vsnprintf (stackbuf, 256, msg, stkargs);
        va_end (stkargs);
        if (nwrit >= 256)
        {
            heapbuf = pctxt->alloc_fn (nwrit + 1);
            if (heapbuf)
            {
                (void) vsnprintf (heapbuf, nwrit + 1, msg, fmtargs);
                dispatch_error (ctxt, code, heapbuf);
                pctxt->free_fn (heapbuf);
            }
            else
                dispatch_error (
                    ctxt, code, "Unable to allocate temporary memory");
        }
        else
            dispatch_error (ctxt, code, stackbuf);
    }
    va_end (fmtargs);
    return code;
}

/**************************************/

static void
internal_exr_destroy_parts (struct _internal_exr_context* ctxt)
{
    uint64_t*              ctable;
    exr_memory_free_func_t dofree = ctxt->free_fn;

    for (int p = 0; p < ctxt->num_parts; ++p)
    {
        struct _internal_exr_part* cur = ctxt->parts[p];

        exr_attr_list_destroy ((exr_context_t) ctxt, &(cur->attributes));

        /* we stack x and y together so only have to free the first */
        if (cur->tile_level_tile_count_x) dofree (cur->tile_level_tile_count_x);

        ctable = (uint64_t*) atomic_load (&(cur->chunk_table));
        if (ctable) dofree (ctable);

        /* the first one is always the one that is part of the file */
        if (p > 0) { dofree (cur); }
        else
        {
            memset (cur, 0, sizeof (struct _internal_exr_part));
        }
    }

    if (ctxt->num_parts > 1) dofree (ctxt->parts);
    ctxt->parts     = NULL;
    ctxt->num_parts = 0;
}

/**************************************/

int
internal_exr_add_part (
    struct _internal_exr_context* f,
    struct _internal_exr_part**   outpart,
    int*                          new_index)
{
    int                         ncount = f->num_parts + 1;
    struct _internal_exr_part*  part;
    struct _internal_exr_part** nptrs = NULL;

    if (new_index) *new_index = f->num_parts;

    if (ncount == 1)
    {
        /* no need to zilch, the parent struct will have already been zero'ed */
        part         = &(f->first_part);
        f->init_part = part;
        nptrs        = &(f->init_part);
    }
    else
    {
        struct _internal_exr_part nil = { 0 };

        part = f->alloc_fn (sizeof (struct _internal_exr_part));
        if (!part)
            return f->standard_error (
                (const exr_context_t) f, EXR_ERR_OUT_OF_MEMORY);

        nptrs = f->alloc_fn (sizeof (struct _internal_exr_part*) * ncount);
        if (!nptrs)
        {
            f->free_fn (part);
            return f->standard_error (
                (const exr_context_t) f, EXR_ERR_OUT_OF_MEMORY);
        }
        *part = nil;
    }

    /* assign appropriately invalid values */
    part->storage_mode         = EXR_STORAGE_LAST_TYPE;
    part->data_window.x_max    = -1;
    part->data_window.y_max    = -1;
    part->display_window.x_max = -1;
    part->display_window.y_max = -1;
    part->chunk_count          = -1;

    /* put it into the part table */
    if (ncount > 1)
    {
        for (int p = 0; p < f->num_parts; ++p)
        {
            nptrs[p] = f->parts[p];
        }
        nptrs[ncount - 1] = part;
    }

    if (f->num_parts > 1) { f->free_fn (f->parts); }
    f->parts     = nptrs;
    f->num_parts = ncount;
    if (outpart) *outpart = part;

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
internal_exr_alloc_context (
    struct _internal_exr_context**   out,
    const exr_context_initializer_t* initializers,
    enum _INTERNAL_EXR_CONTEXT_MODE  mode,
    size_t                           default_size)
{
    void*                         memptr;
    exr_result_t                  rv;
    struct _internal_exr_context* ret;
    *out = NULL;
    int    gmaxw, gmaxh;
    size_t extra_data;

    if (initializers->read_fn || initializers->write_fn)
        extra_data = 0;
    else
        extra_data = default_size;

    memptr = (initializers->alloc_fn) (
        sizeof (struct _internal_exr_context) + extra_data);
    if (memptr)
    {
        memset (memptr, 0, sizeof (struct _internal_exr_context));

        ret       = memptr;
        ret->mode = mode;
        if (initializers->read_fn || initializers->write_fn)
            ret->user_data = initializers->user_data;
        else if (extra_data > 0)
            ret->user_data =
                (((uint8_t*) memptr) + sizeof (struct _internal_exr_context));

        ret->standard_error   = &dispatch_standard_error;
        ret->report_error     = &dispatch_error;
        ret->print_error      = &dispatch_print_error;
        ret->error_handler_fn = initializers->error_handler_fn;
        ret->alloc_fn         = initializers->alloc_fn;
        ret->free_fn          = initializers->free_fn;

        exr_get_default_maximum_image_size (&gmaxw, &gmaxh);
        if (initializers->max_image_width <= 0)
            ret->max_image_w = gmaxw;
        else
            ret->max_image_w = initializers->max_image_width;
        if (ret->max_image_w > 0 && gmaxw > 0 && ret->max_image_w &&
            ret->max_image_w > gmaxw)
            ret->max_image_w = gmaxw;

        if (initializers->max_image_height <= 0)
            ret->max_image_h = gmaxh;
        else
            ret->max_image_h = initializers->max_image_height;
        if (ret->max_image_h > 0 && gmaxh > 0 && ret->max_image_h &&
            ret->max_image_h > gmaxh)
            ret->max_image_h = gmaxh;

        exr_get_default_maximum_tile_size (&gmaxw, &gmaxh);
        if (initializers->max_tile_width <= 0)
            ret->max_tile_w = gmaxw;
        else
            ret->max_tile_w = initializers->max_tile_width;
        if (ret->max_tile_w > 0 && gmaxw > 0 && ret->max_tile_w &&
            ret->max_tile_w > gmaxw)
            ret->max_tile_w = gmaxw;

        if (initializers->max_tile_height <= 0)
            ret->max_tile_h = gmaxh;
        else
            ret->max_tile_h = initializers->max_tile_height;
        if (ret->max_tile_h > 0 && gmaxh > 0 && ret->max_tile_h &&
            ret->max_tile_h > gmaxh)
            ret->max_tile_h = gmaxh;

        ret->file_size       = -1;
        ret->max_name_length = EXR_SHORTNAME_MAXLEN;

        ret->destroy_fn = initializers->destroy_fn;
        ret->read_fn    = initializers->read_fn;
        ret->write_fn   = initializers->write_fn;

#ifdef ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
        InitializeCriticalSection( &(ret->mutex) );
#    else
        rv = pthread_mutex_init (&(ret->mutex), NULL);
        if (rv != 0)
        {
            /* fairly unlikely... */
            (initializers->free_fn) (memptr);
            *out = NULL;
            return EXR_ERR_OUT_OF_MEMORY;
        }
#    endif
#endif

        *out = ret;
        rv   = EXR_ERR_SUCCESS;

        /* if we are reading the file, go ahead and set up the first
         * part to make parsing logic easier */
        if (mode != EXR_CONTEXT_WRITE)
        {
            struct _internal_exr_part* part;
            rv = internal_exr_add_part (ret, &part, NULL);
            if (rv != EXR_ERR_SUCCESS)
            {
                /* this should never happen since we reserve space for
                 * one in the struct, but maybe we changed
                 * something */
                (initializers->free_fn) (memptr);
                *out = NULL;
            }
        }
    }
    else
    {
        (initializers->error_handler_fn) (
            NULL,
            EXR_ERR_OUT_OF_MEMORY,
            exr_get_default_error_message (EXR_ERR_OUT_OF_MEMORY));
        rv = EXR_ERR_OUT_OF_MEMORY;
    }

    return rv;
}

/**************************************/

void
internal_exr_destroy_context (struct _internal_exr_context* ctxt)
{
    exr_memory_free_func_t dofree = ctxt->free_fn;

    exr_attr_string_destroy ((exr_context_t) ctxt, &(ctxt->filename));
    exr_attr_string_destroy ((exr_context_t) ctxt, &(ctxt->tmp_filename));
    exr_attr_list_destroy ((exr_context_t) ctxt, &(ctxt->custom_handlers));
    internal_exr_destroy_parts (ctxt);
#ifdef ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
    DeleteCriticalSection( &(ctxt->mutex) );
#    else
    pthread_mutex_destroy (&(ctxt->mutex));
#    endif
#endif

    dofree (ctxt);
}

/**************************************/

void
internal_exr_update_default_handlers (exr_context_initializer_t* inits)
{
    if (!inits->error_handler_fn)
        inits->error_handler_fn = &default_error_handler;

    if (!inits->alloc_fn) inits->alloc_fn = &internal_exr_alloc;
    if (!inits->free_fn) inits->free_fn = &internal_exr_free;
}
