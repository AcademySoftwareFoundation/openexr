/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_preview.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int exr_attr_preview_init(
    exr_file_t *f, exr_attr_preview_t *p, uint32_t w, uint32_t h )
{
    exr_attr_preview_t nil = {0};
    size_t bytes = (size_t)w * (size_t)h * (size_t)4;

    if ( bytes > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid very large size for preview image (%u x %u - %lu bytes)",
                w, h, bytes );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! p )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to preview object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    
    *p = nil;
    p->rgba = priv_alloc( bytes );
    if ( p->rgba == NULL )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_OUT_OF_MEMORY,
                "Unable to create memory for preview image %u x %u (%lu bytes)",
                w, h, bytes );
        return EXR_ERR_OUT_OF_MEMORY;
    }
    p->alloc_size = bytes;
    p->width = w;
    p->height = h;
    return EXR_ERR_SUCCESS;
}

/**************************************/

int exr_attr_preview_create(
    exr_file_t *f, exr_attr_preview_t *p, uint32_t w, uint32_t h, const uint8_t *d )
{
    int rv = exr_attr_preview_init( f, p, w, h );
    if ( rv == 0 )
    {
        size_t copybytes = w * h * 4;
        if ( p->alloc_size >= copybytes )
            memcpy( (uint8_t *)p->rgba, d, copybytes );
        else
            rv = EXR_ERR_INVALID_ARGUMENT;
    }
    return rv;
}

/**************************************/

void exr_attr_preview_destroy( exr_attr_preview_t *p )
{
    if ( p )
    {
        exr_attr_preview_t nil = {0};
        if ( p->rgba && p->alloc_size > 0 )
            priv_free( (uint8_t *)p->rgba );
        *p = nil;
    }
}
