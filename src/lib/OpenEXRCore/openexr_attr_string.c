/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_string.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int exr_attr_string_init(
    exr_file_t *f, exr_attr_string_t *s, int32_t len )
{
    exr_attr_string_t nil = {0};
    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Received request to allocate negative sized string (%d)",
                len );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to string object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    *s = nil;
    s->str = priv_alloc( len + 1 );
    if ( s->str == NULL )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_OUT_OF_MEMORY,
                "Unable to create memory for string (%d bytes)",
                len + 1 );
        return EXR_ERR_OUT_OF_MEMORY;
    }
    s->length = len;
    s->alloc_size = len + 1;
    return EXR_ERR_SUCCESS;
}

/**************************************/

int exr_attr_string_init_static_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *v, int32_t len )
{
    exr_attr_string_t nil = {0};
    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Received request to allocate negative sized string (%d)",
                len );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! v )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid static string argument to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid reference to string object to initialize" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    *s = nil;
    s->length = len;
    s->str = v;
    return EXR_ERR_SUCCESS;
}

/**************************************/

int exr_attr_string_init_static(
    exr_file_t *f, exr_attr_string_t *s, const char *v )
{
    size_t fulllen = 0;
    int32_t length = 0;
    if ( v )
    {
        fulllen = strlen( v );
        if ( fulllen >= (size_t)INT32_MAX )
        {
            if ( f )
                EXR_GETFILE(f)->report_error(
                    f, EXR_ERR_INVALID_ARGUMENT,
                    "Invalid string too long for attribute" );
            return EXR_ERR_INVALID_ARGUMENT;
        }
        length = (int32_t)fulllen;
    }
    return exr_attr_string_init_static_with_length( f, s, v, length );
}

/**************************************/

int exr_attr_string_create_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *d, int32_t len )
{
    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid (NULL) arguments to string create with length" );
        return EXR_ERR_INVALID_ARGUMENT;
    }
    
    int rv = exr_attr_string_init( f, s, len );
    if ( rv == EXR_ERR_SUCCESS )
    {
        /* we know we own the string memory */
        char *outs = (char *)s->str;
        /* someone might pass in a string shorter than requested length (or longer) */
        if ( len > 0 )
        {
            if ( d )
                strncpy( outs, d, len );
            else
                memset( outs, 0, len );
        }
        outs[len] = '\0';
    }
    return rv;
}

/**************************************/

int exr_attr_string_create(
    exr_file_t *f, exr_attr_string_t *s, const char *d )
{
    int32_t len = 0;
    if ( d )
        len = (int32_t)strlen( d );
    return exr_attr_string_create_with_length( f, s, d, len );
}

/**************************************/

int exr_attr_string_set_with_length(
    exr_file_t *f, exr_attr_string_t *s, const char *d, int32_t len )
{
    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Invalid string argument to string set" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_ERR_INVALID_ARGUMENT,
                "Received request to assign a negative sized string (%d)",
                len );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( s->alloc_size > len )
    {
        s->length = len;
        /* we own the memory */
        char *sstr = (char *)s->str;
        if ( len > 0 )
        {
            if ( d )
                strncpy( sstr, d, len );
            else
                memset( sstr, 0, len );
        }
        sstr[len] = '\0';
        return EXR_ERR_SUCCESS;
    }
    exr_attr_string_destroy( s );
    return exr_attr_string_create_with_length( f, s, d, len );
}

/**************************************/

int exr_attr_string_set(
    exr_file_t *f, exr_attr_string_t *s, const char *d )
{
    int32_t len = 0;
    if ( d )
        len = (int32_t)strlen( d );
    return exr_attr_string_set_with_length( f, s, d, len );
}

/**************************************/

void exr_attr_string_destroy( exr_attr_string_t *s )
{
    if ( s )
    {
        exr_attr_string_t nil = {0};
        if ( s->str && s->alloc_size > 0 )
            priv_free( (char *)s->str );
        *s = nil;
    }
}
