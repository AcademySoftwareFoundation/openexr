/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_float_vector.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

/* allocates ram, but does not fill any data */
int exr_attr_float_vector_init(
    exr_file_t *f, exr_attr_float_vector_t *fv, int32_t nent )
{
    exr_attr_float_vector_t nil = {0};
    size_t bytes = (size_t)nent * sizeof(float);

    if ( nent < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Received request to allocate negative sized float vector (%d entries)",
                nent );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    if ( bytes > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid too large size for float vector (%d entries)",
                nent );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! fv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to float vector object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    *fv = nil;
    if ( bytes > 0 )
    {
        fv->arr = priv_alloc( bytes );
        if ( fv->arr == NULL )
        {
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_ERR_OUT_OF_MEMORY,
                    "Unable to create memory for float vector (%lu bytes)",
                    bytes );
            return EXR_ERR_OUT_OF_MEMORY;
        }
        fv->length = nent;
        fv->alloc_size = nent;
    }
        
    return EXR_ERR_SUCCESS;
}

/**************************************/

int exr_attr_float_vector_init_static(
    exr_file_t *f, exr_attr_float_vector_t *fv, const float *arr, int32_t nent )
{
    exr_attr_float_vector_t nil = {0};

    if ( nent < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Received request to allocate negative sized float vector (%d entries)",
                nent );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    if ( ! fv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to float vector object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    if ( ! arr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to float array object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    *fv = nil;
    fv->arr = arr;
    fv->length = nent;
    fv->alloc_size = 0;
    return EXR_ERR_SUCCESS;
}

/**************************************/

int exr_attr_float_vector_create(
    exr_file_t *f, exr_attr_float_vector_t *fv, const float *arr, int32_t nent )
{
    int rv = EXR_ERR_UNKNOWN;
    if ( ! fv || ! arr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid (NULL) arguments to float vector create" );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    
    rv = exr_attr_float_vector_init( f, fv, nent );
    if ( rv == 0 && nent > 0 )
        memcpy( (float *)fv->arr, arr, nent * sizeof(float) );
    return rv;
}

/**************************************/

void exr_attr_float_vector_destroy( exr_attr_float_vector_t *fv )
{
    if ( fv )
    {
        exr_attr_float_vector_t nil = {0};
        if ( fv->arr && fv->alloc_size > 0 )
            priv_free( (void *)fv->arr );
        *fv = nil;
    }
}
