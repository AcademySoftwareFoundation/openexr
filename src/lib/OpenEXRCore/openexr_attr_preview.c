/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_preview.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int EXR_FUN(attr_preview_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_preview) *p, uint32_t w, uint32_t h )
{
    EXR_TYPE(attr_preview) nil = {0};
    size_t bytes = (size_t)w * (size_t)h * (size_t)4;

    if ( bytes > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid very large size for preview image (%u x %u - %lu bytes)",
                w, h, bytes );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! p )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to preview object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    *p = nil;
    p->rgba = priv_alloc( bytes );
    if ( p->rgba == NULL )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_OUT_OF_MEMORY),
                "Unable to create memory for preview image %u x %u (%lu bytes)",
                w, h, bytes );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }
    p->alloc_size = bytes;
    p->width = w;
    p->height = h;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_preview_create)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_preview) *p, uint32_t w, uint32_t h, const uint8_t *d )
{
    int rv = EXR_FUN(attr_preview_init)( f, p, w, h );
    if ( rv == 0 )
    {
        size_t copybytes = w * h * 4;
        if ( p->alloc_size >= copybytes )
            memcpy( (uint8_t *)p->rgba, d, copybytes );
        else
            rv = EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    return rv;
}

/**************************************/

void EXR_FUN(attr_preview_destroy)( EXR_TYPE(attr_preview) *p )
{
    if ( p )
    {
        EXR_TYPE(attr_preview) nil = {0};
        if ( p->rgba && p->alloc_size > 0 )
            priv_free( (uint8_t *)p->rgba );
        *p = nil;
    }
}
