/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_opaque.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int EXR_FUN(attr_opaquedata_init)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_opaquedata) *u, size_t b )
{
    EXR_TYPE(attr_opaquedata) nil = {0};
    if ( ! u )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to opaque data object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }
    
    if ( b > (size_t)INT32_MAX )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid size for opaque data (%lu bytes, must be <= INT32_MAX)",
                b );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *u = nil;
    u->packed_data = priv_alloc( b );
    if ( ! u->packed_data )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_OUT_OF_MEMORY),
                "Unable to create memory for opaque data of %lu bytes",
                b );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }
    u->size = b;
    u->alloc_size = b;
    return EXR_DEF(ERR_SUCCESS);

}

/**************************************/

int EXR_FUN(attr_opaquedata_create)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_opaquedata) *u, size_t b, const void *d )
{
    int rv = EXR_FUN(attr_opaquedata_init)( f, u, b );
    if ( rv == 0 )
    {
        if ( d )
            memcpy( (void *)u->packed_data, d, b );
    }

    return rv;
}

/**************************************/

void EXR_FUN(attr_opaquedata_destroy)( EXR_TYPE(attr_opaquedata) *ud )
{
    if ( ud )
    {
        EXR_TYPE(attr_opaquedata) nil = {0};
        if ( ud->packed_data )
        {
            if ( ud->alloc_size > 0 )
                priv_free( ud->packed_data );
            else if ( ud->unpacked_data && ud->destroy_func_ptr )
                ud->destroy_func_ptr( ud->packed_data, ud->size );
        }

        if ( ud->unpacked_data )
        {
            if ( ud->destroy_func_ptr )
                ud->destroy_func_ptr( ud->unpacked_data, ud->unpacked_size );
            else
                priv_free( ud->unpacked_data );
        }
        *ud = nil;
    }
}

/**************************************/

void *EXR_FUN(attr_opaquedata_unpack)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_opaquedata) *u, int32_t *sz )
{
    if ( sz )
        *sz = 0;

    if ( ! u )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to opaque data object to initialize" );
        return NULL;
    }

    if ( u->unpacked_data )
    {
        if ( sz )
            *sz = u->unpacked_size;
        return u->unpacked_data;
    }

    if ( ! u->unpack_func_ptr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "No unpack provider specified for opaque data" );
        return NULL;
    }
    u->unpack_func_ptr( u->packed_data, u->size, &(u->unpacked_size), &(u->unpacked_data) );
    if ( sz )
        *sz = u->unpacked_size;
    return u->unpacked_data;
}

/**************************************/

void *EXR_FUN(attr_opaquedata_pack)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_opaquedata) *u, int32_t *sz )
{
    int rv = EXR_DEF(ERR_SUCCESS);
    int32_t nsize = 0;
    void *tmpptr = NULL;
    if ( sz )
        *sz = 0;

    if ( ! u )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to opaque data object to initialize" );
        return NULL;
    }

    if ( u->packed_data )
    {
        if ( sz )
            *sz = u->size;
        return u->packed_data;
    }

    if ( ! u->pack_func_ptr )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "No pack provider specified for opaque data" );
        return NULL;
    }
    rv = u->pack_func_ptr( u->unpacked_data, u->unpacked_size, &nsize, &tmpptr );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        return NULL;
    }
    
    u->size = nsize;
    u->alloc_size = u->size;
    u->packed_data = tmpptr;
    if ( sz )
        *sz = u->size;
    return u->packed_data;
}

/**************************************/

int EXR_FUN(attr_opaquedata_set_unpacked)(
    EXR_TYPE(FILE) *f, EXR_TYPE(attr_opaquedata) *u, void *unpacked, int32_t sz )
{
    if ( ! u )
    {
        if ( f )
            EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid reference to opaque data object to initialize" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( u->unpacked_data )
    {
        if ( u->destroy_func_ptr )
            u->destroy_func_ptr( u->unpacked_data, u->unpacked_size );
        else
            priv_free( u->unpacked_data );
    }
    u->unpacked_data = unpacked;
    u->unpacked_size = sz;

    if ( u->packed_data )
    {
        if ( u->alloc_size > 0 )
            priv_free( u->packed_data );
        u->packed_data = NULL;
        u->size = 0;
    }

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(register_attr_handler)(
    EXR_TYPE(FILE) *file, const char *type,
    int (*unpack_func_ptr)( const void *data, int32_t attrsize, int32_t *outsize, void **outbuffer ),
    int (*pack_func_ptr)( const void *data, int32_t datasize, int32_t *outsize, void **outbuffer ),
    void (*destroy_func_ptr)( void *data, int32_t datasize ) )
{
    EXR_TYPE(PRIV_FILE) *f = EXR_GETFILE(file);
    EXR_TYPE(attribute) *ent;
    int rv;
    int32_t nlen, tlen, mlen = EXR_SHORTNAME_MAXLEN;
    char *ptr = NULL;
    size_t attrblocksz = sizeof(EXR_TYPE(attribute));
    int partcount = 0;
    EXR_TYPE(attribute_list) *curattrs;

    if ( ! f )
        return EXR_DEF(ERR_INVALID_ARGUMENT);

    mlen = (int32_t)EXR_GETFILE(f)->max_name_length;

    if ( ! type || type[0] == '\0' )
        return EXR_GETFILE(f)->report_error(
            f, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Invalid type to register_attr_handler" );

    tlen = (int32_t)strlen( type );
    if ( tlen > mlen )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Provided type name '%s' too long for file (len %d, max %d)",
            type, nlen, mlen );

    ent = EXR_FUN(attr_list_find_by_name)( f, &(f->custom_handlers), type );
    if ( ent )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Attribute handler for '%s' previously registered",
            type );
    ent = NULL;
    rv = EXR_FUN(attr_list_add_by_type)( file, &(f->custom_handlers), type, type, 0, NULL, &ent );
    if ( rv == EXR_DEF(ERR_SUCCESS) && ent )
    {
        ent->opaque->unpack_func_ptr = unpack_func_ptr;
        ent->opaque->pack_func_ptr = pack_func_ptr;
        ent->opaque->destroy_func_ptr = destroy_func_ptr;
    }
    else
    {
        return EXR_GETFILE(f)->print_error(
            f, rv, "Unable to register custom handler for type '%s'", type );
    }

    partcount = EXR_FUN(get_part_count)( file );
    for ( int p = 0; p < partcount; ++p )
    {
        curattrs = EXR_FUN(get_attribute_list)( file, p );
        if ( curattrs )
        {
            int nattr = curattrs->num_attributes;
            for ( int a = 0; a < nattr; ++a )
            {
                ent = curattrs->entries[a];
                if ( ent->type_name_length == tlen &&
                     0 == strcmp( ent->type_name, type ) )
                {
                    ent->opaque->unpack_func_ptr = unpack_func_ptr;
                    ent->opaque->pack_func_ptr = pack_func_ptr;
                    ent->opaque->destroy_func_ptr = destroy_func_ptr;
                }
            }
        }
    }
    
    return rv;
}
