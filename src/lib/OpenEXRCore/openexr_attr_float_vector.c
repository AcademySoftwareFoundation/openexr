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
int EXR_FUN(attr_float_vector_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, int32_t nent )
{
    EXR_TYPE(attr_float_vector) nil = {0};
    size_t bytes = (size_t)nent * sizeof(float);

    if ( nent < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to allocate negative sized float vector (%d entries)",
                nent );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    if ( bytes > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid too large size for float vector (%d entries)",
                nent );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! fv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to float vector object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *fv = nil;
    if ( bytes > 0 )
    {
        fv->arr = priv_alloc( bytes );
        if ( fv->arr == NULL )
        {
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Unable to create memory for float vector (%lu bytes)",
                    bytes );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        fv->length = nent;
        fv->alloc_size = nent;
    }
        
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_float_vector_init_static)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, const float *arr, int32_t nent )
{
    EXR_TYPE(attr_float_vector) nil = {0};

    if ( nent < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to allocate negative sized float vector (%d entries)",
                nent );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    if ( ! fv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to float vector object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    if ( ! arr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to float array object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *fv = nil;
    fv->arr = arr;
    fv->length = nent;
    fv->alloc_size = 0;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_float_vector_create)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_float_vector) *fv, const float *arr, int32_t nent )
{
    int rv = EXR_DEF(ERR_UNKNOWN);
    if ( ! fv || ! arr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid (NULL) arguments to float vector create" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    rv = EXR_FUN(attr_float_vector_init)( f, fv, nent );
    if ( rv == 0 && nent > 0 )
        memcpy( (float *)fv->arr, arr, nent * sizeof(float) );
    return rv;
}

/**************************************/

void EXR_FUN(attr_float_vector_destroy)( EXR_TYPE(attr_float_vector) *fv )
{
    if ( fv )
    {
        EXR_TYPE(attr_float_vector) nil = {0};
        if ( fv->arr && fv->alloc_size > 0 )
            priv_free( (void *)fv->arr );
        *fv = nil;
    }
}
