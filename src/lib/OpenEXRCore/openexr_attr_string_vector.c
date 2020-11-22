/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_string_vector.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int EXR_FUN(attr_string_vector_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t nent )
{
    EXR_TYPE(attr_string_vector) nil = {0};
    EXR_TYPE(attr_string) nils = {0};
    size_t bytes = (size_t)nent * sizeof(EXR_TYPE(attr_string));

    if ( ! sv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to string vector object to assign to" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    if ( nent < 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Received request to allocate negative sized string vector (%d entries)",
                nent );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    if ( bytes > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid too large size for string vector (%d entries)",
                nent );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *sv = nil;
    if ( bytes > 0 )
    {
        sv->strings = priv_alloc( bytes );
        if ( sv->strings == NULL )
        {
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Unable to create memory for string vector (%lu bytes)",
                    bytes );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        sv->n_strings = nent;
        sv->alloc_size = nent;
        for ( int32_t i = 0; i < nent; ++i )
            *((EXR_TYPE(attr_string) *)(sv->strings + i)) = nils;
    }
        
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

void EXR_FUN(attr_string_vector_destroy)( EXR_TYPE(attr_string_vector) *sv )
{
    if ( sv )
    {
        EXR_TYPE(attr_string_vector) nil = {0};
        if ( sv->alloc_size > 0 )
        {
            EXR_TYPE(attr_string) *strs = (EXR_TYPE(attr_string) *)sv->strings;
            for ( int32_t i = 0; i < sv->n_strings; ++i )
                EXR_FUN(attr_string_destroy)( strs + i );
            if ( strs )
                priv_free( strs );
        }
        *sv = nil;
    }
}

/**************************************/

int EXR_FUN(attr_string_vector_init_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, int32_t len )
{
    if ( sv )
    {
        if ( idx < 0 || idx >= sv->n_strings )
        {
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ARGUMENT),
                    "Invalid index (%d of %d) initializing string vector",
                    idx, sv->n_strings );
            return EXR_DEF(ERR_INVALID_ARGUMENT);
        }

        return EXR_FUN(attr_string_init)( f, (EXR_TYPE(attr_string) *)sv->strings + idx, len );
    }
    
    if ( f )
        EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Invalid reference to string vector object to initialize index %d",
            idx );
    return EXR_DEF(ERR_INVALID_ARGUMENT);
}

/**************************************/

int EXR_FUN(attr_string_vector_set_entry_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, const char *s, int32_t len )
{
    if ( ! sv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to string vector object to assign to" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    if ( idx < 0 || idx >= sv->n_strings )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid index (%d of %d) assigning string vector ('%s', len %d)",
                idx, sv->n_strings, s ? s : "<nil>", len );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    return EXR_FUN(attr_string_set_with_length)( f, (EXR_TYPE(attr_string) *)sv->strings + idx, s, len );
}

/**************************************/

int EXR_FUN(attr_string_vector_set_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, int32_t idx, const char *s )
{
    int32_t len = 0;
    if ( s )
        len = (int32_t)strlen( s );
    return EXR_FUN(attr_string_vector_set_entry_with_length)( f, sv, idx, s, len );
}

/**************************************/

int EXR_FUN(attr_string_vector_add_entry_with_length)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, const char *s, int32_t len )
{
    int32_t nent;
    int rv;
    EXR_TYPE(attr_string) *nlist;

    if ( ! sv )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to string vector object to assign to" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    nent = sv->n_strings + 1;
    if ( nent > sv->alloc_size )
    {
        EXR_TYPE(attr_string) nil = {0};
        size_t bytes;
        int32_t allsz = sv->alloc_size * 2;

        if ( nent > allsz )
            allsz = nent + 1;
        bytes = allsz * sizeof(EXR_TYPE(attr_string));
        nlist = (EXR_TYPE(attr_string) *)priv_alloc( bytes );
        if ( nlist == NULL )
        {
            if ( f )
                EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Unable to create memory for string vector (%lu bytes)",
                    bytes );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        for ( int32_t i = 0; i < sv->n_strings; ++i )
            *(nlist + i) = sv->strings[i];
        if ( sv->alloc_size > 0 )
            priv_free( (void *)sv->strings );
        sv->strings = nlist;
        sv->alloc_size = allsz;
    }
    else
    {
        /* that means we own this and can write into, cast away const */
        nlist = (EXR_TYPE(attr_string) *)sv->strings;
    }

    rv = EXR_FUN(attr_string_create_with_length)(
        f, nlist + sv->n_strings, s, len );
    if ( rv == 0 )
        sv->n_strings = nent;
    return rv;
}

/**************************************/

int EXR_FUN(attr_string_vector_add_entry)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_string_vector) *sv, const char *s )
{
    int32_t len = 0;
    if ( s )
        len = (int32_t)strlen( s );
    return EXR_FUN(attr_string_vector_add_entry_with_length)( f, sv, s, len );
}
