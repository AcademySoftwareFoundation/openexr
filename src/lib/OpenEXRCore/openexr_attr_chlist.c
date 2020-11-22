/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_attr_chlist.h"

#include "openexr_priv_structs.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#include <string.h>

/**************************************/

int EXR_FUN(attr_chlist_init)( EXR_TYPE(FILE) *f,
                               EXR_TYPE(attr_chlist) *clist,
                               int nchans )
{
    EXR_TYPE(attr_chlist) nil = {0};
    EXR_TYPE(attr_chlist_entry) *nlist;

    if ( ! clist )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid channel list pointer to chlist_add_with_length" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( nchans < 0 )
    {
        if ( f )
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Negative number of channels requested (%d)", nchans );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *clist = nil;

    nlist = priv_alloc( sizeof(*nlist) * nchans );
    if ( nlist == NULL )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_OUT_OF_MEMORY),
                "Unable to alloc memory for channel list" );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }
    clist->entries = nlist;
    clist->num_alloced = nchans;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int EXR_FUN(attr_chlist_add)( EXR_TYPE(FILE) *f,
                              EXR_TYPE(attr_chlist) *clist,
                              const char *name,
                              EXR_TYPE(PIXEL_TYPE) ptype,
                              uint8_t islinear,
                              int32_t xsamp,
                              int32_t ysamp )
{
    int32_t len = 0;
    if ( name )
        len = (int32_t)strlen( name );
    return EXR_FUN(attr_chlist_add_with_length)( f, clist, name, len, ptype, islinear, xsamp, ysamp );
}

/**************************************/

int EXR_FUN(attr_chlist_add_with_length)( EXR_TYPE(FILE) *f,
                                          EXR_TYPE(attr_chlist) *clist,
                                          const char *name,
                                          int32_t namelen,
                                          EXR_TYPE(PIXEL_TYPE) ptype,
                                          uint8_t islinear,
                                          int32_t xsamp,
                                          int32_t ysamp )
{
    EXR_TYPE(attr_chlist_entry) nent = {0};
    EXR_TYPE(attr_chlist_entry) *nlist, *olist;
    int newcount, insertpos;
    int32_t maxlen = ( f ) ? EXR_GETFILE(f)->max_name_length : EXR_SHORTNAME_MAXLEN;

    if ( ! clist )
    {
        if ( f )
            return EXR_GETFILE(f)->report_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid channel list pointer to chlist_add_with_length" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! name || name[0] == '\0' || namelen == 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Channel name must not be empty, received '%s'", (name ? name : "<NULL>") );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( namelen > maxlen )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Channel name must shorter than length allowed by file (%d), received '%s' (%d)",
                maxlen, (name ? name : "<NULL>"), namelen );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ptype != EXR_DEF(PIXEL_UINT) && ptype != EXR_DEF(PIXEL_HALF) && ptype != EXR_DEF(PIXEL_FLOAT) )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid pixel type specified (%d) adding channel '%s' to list", (int)ptype, name );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( islinear >= 2 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid linear flag value (%d) adding channel '%s' to list", (int)islinear, name );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( xsamp <= 0 || ysamp <= 0 )
    {
        if ( f )
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ARGUMENT),
                "Invalid pixel sampling (x %d y %d) adding channel '%s' to list", xsamp, ysamp, name );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    insertpos = 0;
    olist = clist->entries;
    for ( int32_t c = 0; c < clist->num_channels; ++c )
    {
        int ord = strcmp( name, olist[c].name.str );
        if ( ord < 0 )
        {
            insertpos = c;
            break;
        }
        else if ( ord == 0 )
        {
            if ( f )
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ARGUMENT),
                    "Attempt to add duplicate channel '%s' to channel list",
                    name );
            return EXR_DEF(ERR_INVALID_ARGUMENT);
        }
        else
            insertpos = c + 1;
    }

    /* temporarily use newcount as a return value check */
    newcount = EXR_FUN(attr_string_create_with_length)( f, &(nent.name), name, namelen );
    if ( newcount != EXR_DEF(ERR_SUCCESS) )
        return newcount;

    newcount = clist->num_channels + 1;
    nent.pixel_type = ptype;
    nent.p_linear = islinear;
    nent.x_sampling = xsamp;
    nent.y_sampling = ysamp;

    if ( newcount > clist->num_alloced )
    {
        int nsz = clist->num_alloced * 2;
        if ( newcount > nsz )
            nsz = newcount + 1;
        nlist = priv_alloc( sizeof(*nlist) * nsz );
        if ( nlist == NULL )
        {
            EXR_FUN(attr_string_destroy)( &(nent.name) );
            if ( f )
                return EXR_GETFILE(f)->report_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Unable to alloc memory for channel list" );
            return EXR_DEF(ERR_OUT_OF_MEMORY);
        }
        clist->num_alloced = nsz;
    }
    else
        nlist = clist->entries;

    /* since we can re-use same memory, have to have slightly more
     * complex logic to avoid overwrites, find where we will insert
     * and copy entries after that first */

    /* shift old entries further first */
    for ( int i = newcount - 1; i > insertpos; --i )
        nlist[i] = olist[i - 1];
    nlist[insertpos] = nent;
    if ( nlist != olist )
    {
        for ( int i = 0; i < insertpos; ++i )
            nlist[i] = olist[i];
    }

    clist->num_channels = newcount;
    clist->entries = nlist;
    if ( nlist != olist )
        priv_free( olist );
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

void EXR_FUN(attr_chlist_destroy)( EXR_TYPE(attr_chlist) *clist )
{
    if ( clist )
    {
        EXR_TYPE(attr_chlist) nil = {0};
        int nc = clist->num_channels;
        EXR_TYPE(attr_chlist_entry) *entries = clist->entries;

        for ( int i = 0; i < nc; ++i )
            EXR_FUN(attr_string_destroy)( &(entries[i].name) );
        if ( entries )
            priv_free( entries );
        *clist = nil;
    }
}
