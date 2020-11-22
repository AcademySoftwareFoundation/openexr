/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_string.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int EXR_FUN(attr_string_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, int32_t len )
{
    EXR_TYPE(attr_string) nil = {0};
    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to allocate negative sized string (%d)",
                len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to string object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *s = nil;
    s->str = priv_alloc( len + 1 );
    if ( s->str == NULL )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_OUT_OF_MEMORY),
                "Unable to create memory for string (%d bytes)",
                len + 1 );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }
    s->length = len;
    s->alloc_size = len + 1;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_string_init_static_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *v, int32_t len )
{
    EXR_TYPE(attr_string) nil = {0};
    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to allocate negative sized string (%d)",
                len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! v )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid static string argument to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to string object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *s = nil;
    s->length = len;
    s->str = v;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_string_init_static)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *v )
{
    int32_t length = 0;
    if ( v )
        length = strlen( v );
    return EXR_FUN(attr_string_init_static_with_length)( f, s, v, length );
}

/**************************************/

int EXR_FUN(attr_string_create_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *d, int32_t len )
{
    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid (NULL) arguments to string create with length" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    int rv = EXR_FUN(attr_string_init)( f, s, len );
    if ( rv == EXR_DEF(ERR_SUCCESS) )
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

int EXR_FUN(attr_string_create)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *d )
{
    int32_t len = 0;
    if ( d )
        len = (int32_t)strlen( d );
    return EXR_FUN(attr_string_create_with_length)( f, s, d, len );
}

/**************************************/

int EXR_FUN(attr_string_set_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *d, int32_t len )
{
    if ( ! s )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid string argument to string set" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( len < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to assign a negative sized string (%d)",
                len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
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
        return EXR_DEF(ERR_SUCCESS);
    }
    EXR_FUN(attr_string_destroy)( s );
    return EXR_FUN(attr_string_create_with_length)( f, s, d, len );
}

/**************************************/

int EXR_FUN(attr_string_set)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string) *s, const char *d )
{
    int32_t len = 0;
    if ( d )
        len = (int32_t)strlen( d );
    return EXR_FUN(attr_string_set_with_length)( f, s, d, len );
}

/**************************************/

void EXR_FUN(attr_string_destroy)( EXR_TYPE(attr_string) *s )
{
    if ( s )
    {
        EXR_TYPE(attr_string) nil = {0};
        if ( s->str && s->alloc_size > 0 )
            priv_free( (char *)s->str );
        *s = nil;
    }
}
