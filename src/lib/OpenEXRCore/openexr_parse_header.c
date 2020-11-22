/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_priv_file.h"

#include "openexr_priv_xdr.h"

#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/**************************************/

typedef struct EXR_TYPE(seq_scratch)
{
    uint8_t *scratch;
    size_t curpos;
    ssize_t navail;
    off_t fileoff;

    int (*sequential_read)( struct EXR_TYPE(seq_scratch) *, void *, size_t );

    EXR_TYPE(FILE) *file;
} EXR_TYPE(PRIV_SEQ_SCRATCH);

#define SCRATCH_BUFFER_SIZE 4096

static int scratch_seq_read( struct EXR_TYPE(seq_scratch) *scr, void *buf, size_t sz )
{
    uint8_t *outbuf = buf;
    size_t nCopied = 0;
    size_t notdone = sz;
    int rv = -1;

    while ( notdone > 0 )
    {
        if ( scr->navail > 0 )
        {
            size_t nLeft = (size_t)scr->navail;
            size_t nCopy = notdone;
            if ( nCopy > nLeft )
                nCopy = nLeft;
            memcpy( outbuf, scr->scratch + scr->curpos, nCopy );
            scr->curpos += nCopy;
            scr->navail -= (ssize_t)nCopy;
            notdone -= nCopy;
            outbuf += nCopy;
            nCopied += nCopy;
        }
        else if ( notdone > SCRATCH_BUFFER_SIZE )
        {
            size_t nPages = notdone / SCRATCH_BUFFER_SIZE;
            ssize_t nread = 0;
            size_t nToRead = nPages * SCRATCH_BUFFER_SIZE;
            rv = EXR_GETFILE(scr->file)->do_read(
                scr->file, outbuf, nToRead, &(scr->fileoff), &nread, EXR_MUST_READ_ALL );
            if ( nread > 0 )
            {
                notdone -= nread;
                outbuf += nread;
                nCopied += nread;
            }
            if ( nread <= 0 )
                break;
        }
        else
        {
            ssize_t nread = 0;
            rv = EXR_GETFILE(scr->file)->do_read(
                scr->file, scr->scratch, SCRATCH_BUFFER_SIZE, &(scr->fileoff), &nread, EXR_ALLOW_SHORT_READ );
            if ( nread > 0 )
            {
                scr->navail = nread;
                scr->curpos = 0;
            }
            else
            {
                break;
            }
        }
    }
    if ( rv == -1 )
    {
        if ( nCopied == sz )
            rv = EXR_DEF(ERR_SUCCESS);
        else
            rv = EXR_DEF(ERR_READ_IO);
    }
    return rv;
}

/**************************************/

static int priv_init_scratch( EXR_TYPE(FILE) *file, EXR_TYPE(PRIV_SEQ_SCRATCH) *scr, off_t offset )
{
    EXR_TYPE(PRIV_SEQ_SCRATCH) nil = {0};
    *scr = nil;
    scr->scratch = priv_alloc( SCRATCH_BUFFER_SIZE );
    if ( scr->scratch == NULL )
        return EXR_GETFILE(file)->report_error(
            file, EXR_DEF(ERR_OUT_OF_MEMORY), "Unable to allocate scratch buffer" );
    scr->sequential_read = &scratch_seq_read;
    scr->fileoff = offset;
    scr->file = file;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static void priv_destroy_scratch( EXR_TYPE(PRIV_SEQ_SCRATCH) *scr )
{
    if ( scr->scratch )
        priv_free( scr->scratch );
}

/**************************************/

static int32_t check_bad_attrsz( EXR_TYPE(FILE) *f, int attrsz, int eltsize, const char *aname, const char *tname )
{
    ssize_t fsize = EXR_GETFILE(f)->file_size;
    int32_t n = attrsz;

    if ( attrsz < 0 )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s', type '%s': Invalid negative size %d",
            aname, tname, attrsz );
    if ( fsize > 0 && attrsz > fsize )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s', type '%s': Invalid size %d",
            aname, tname, attrsz );

    if ( eltsize > 1 )
    {
        n = attrsz / eltsize;
        if ( attrsz != (int32_t)( n * eltsize ) )
            return - EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_SIZE_MISMATCH),
                "Attribute '%s': Invalid size %d (exp '%s' size 4 * n, found odd bytes %d)",
                aname, attrsz, tname, ( attrsz % eltsize ) );
    }

    return n;
}

/**************************************/

static int read_text( EXR_TYPE(PRIV_FILE) *file,
                      char text[256],
                      int32_t *outlen,
                      int32_t maxlen,
                      EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                      const char *type )
{
    char b;
    int rv;
    int32_t namelen = *outlen;
    while ( namelen <= maxlen )
    {
        rv = scratch->sequential_read( scratch, &b, 1 );
        if ( rv )
            return rv;
        if ( b > 0 && ( b > 126 || (b < ' ' && b != '\t' ) ) )
        {
            continue;
            //return EXR_GETFILE(file)->print_error(
            //    file, EXR_DEF(ERR_FILE_BAD_HEADER),
            //    "Invalid non-printable character %d (0x%02X) encountered parsing attribute text", (int)b, (int)b );
        }
        text[namelen] = b;
        if ( b == '\0' )
            break;
        ++namelen;
    }
    *outlen = namelen;
    if ( namelen > maxlen )
        return EXR_GETFILE(file)->print_error(
            file, EXR_DEF(ERR_NAME_TOO_LONG),
            "Invalid %s encountered: start '%s' (max %d)", type, text, maxlen );
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int extract_attr_chlist( EXR_TYPE(FILE) *f,
                                EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                EXR_TYPE(attr_chlist) *attrdata,
                                const char *aname,
                                const char *tname,
                                int32_t attrsz )
{
    char chname[256];
    int32_t chlen;
    int32_t ptype, xsamp, ysamp;
    uint8_t flags[4];
    int32_t maxlen = EXR_GETFILE(f)->max_name_length;

    if ( attrsz <= 0 )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp at least 1 byte for '%s')",
            aname, attrsz, tname );

    while ( attrsz > 0 )
    {
        chlen = 0;
        if ( read_text( f, chname, &chlen, maxlen, scratch, aname ) )
            return -1;
        attrsz -= chlen + 1;

        if ( chlen == 0 )
            break;

        if ( attrsz < 16 )
        {
            return - EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_SIZE_MISMATCH),
                "Out of data parsing '%s', last channel '%s'",
                aname, chname );
        }
        if ( scratch->sequential_read( scratch, &ptype, 4 ) )
            return -1;
        if ( scratch->sequential_read( scratch, &flags, 4 ) )
            return -1;
        if ( scratch->sequential_read( scratch, &xsamp, 4 ) )
            return -1;
        if ( scratch->sequential_read( scratch, &ysamp, 4 ) )
            return -1;

        attrsz -= 16;
        ptype = (int32_t)one_to_native32( (uint32_t)ptype );
        xsamp = (int32_t)one_to_native32( (uint32_t)xsamp );
        ysamp = (int32_t)one_to_native32( (uint32_t)ysamp );

        if ( EXR_FUN(attr_chlist_add_with_length)(
                 f, attrdata, chname, chlen,
                 (EXR_TYPE(PIXEL_TYPE))ptype, flags[0], xsamp, ysamp ) )
        {
            return -1;
        }
    }
    return 0;
}

/**************************************/

static int extract_attr_uint8( EXR_TYPE(FILE) *f,
                               EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                               uint8_t *attrdata,
                               const char *aname,
                               const char *tname,
                               int32_t attrsz,
                               uint8_t maxval )
{
    if ( attrsz != 1 )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp '%s' size 1)",
            aname, attrsz, tname );

    if ( scratch->sequential_read( scratch, attrdata, sizeof(uint8_t) ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Unable to read '%s' %s data",
            aname, tname );

    if ( *attrdata >= maxval )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Attribute '%s' (type '%s'): Invalid value %d (max allowed %d)",
            aname, tname, (int)*attrdata, (int)maxval );

    return 0;
}

/**************************************/

static int extract_attr_64bit( EXR_TYPE(FILE) *f,
                               EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                               void *attrdata,
                               const char *aname,
                               const char *tname,
                               int32_t attrsz,
                               int32_t num )
{
    if ( attrsz != 8 * num )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp '%s' size 8 * %d (%d))",
            aname, attrsz, tname, num, 8 * num );

    if ( scratch->sequential_read( scratch, attrdata, 8 * num ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Unable to read '%s' %s data",
            aname, tname );

    priv_to_native64( attrdata, num );
    return 0;
}

/**************************************/

static int extract_attr_32bit( EXR_TYPE(FILE) *f,
                               EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                               void *attrdata,
                               const char *aname,
                               const char *tname,
                               int32_t attrsz,
                               int32_t num )
{
    if ( attrsz != 4 * num )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp '%s' size 4 * %d (%d))",
            aname, attrsz, tname, num, 4 * num );

    if ( scratch->sequential_read( scratch, attrdata, 4 * num ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Unable to read '%s' %s data",
            aname, tname );

    priv_to_native32( attrdata, num );
    return 0;
}

/**************************************/

static int extract_attr_float_vector( EXR_TYPE(FILE) *f,
                                      EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                      EXR_TYPE(attr_float_vector) *attrdata,
                                      const char *aname,
                                      const char *tname,
                                      int32_t attrsz )
{
    int32_t n = check_bad_attrsz( f, attrsz, (int)sizeof(float), aname, tname );
    if ( n > 0 )
    {
        if ( EXR_FUN(attr_float_vector_init)( f, attrdata, n ) )
            return 1;

        if ( scratch->sequential_read( scratch, (void *)attrdata->arr, (size_t)attrsz ) )
        {
            EXR_FUN(attr_float_vector_destroy)( attrdata );
            return EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_READ_IO),
                "Unable to read '%s' %s data",
                aname, tname );
        }

        priv_to_native32( attrdata, n );
    }
    return n < 0 ? -n : 0;
}

/**************************************/

static int extract_attr_string( EXR_TYPE(FILE) *f,
                                EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                EXR_TYPE(attr_string) *attrdata,
                                const char *aname,
                                const char *tname,
                                int32_t attrsz,
                                char *strptr )
{
    int32_t n = check_bad_attrsz( f, attrsz, 1, aname, tname );
    if ( n >= 0 )
    {
        if ( n > 0 )
        {
            if ( scratch->sequential_read( scratch, (void *)strptr, (size_t)n ) )
            {
                return EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_READ_IO),
                    "Unable to read '%s' %s data",
                    aname, tname );
            }
        }
        strptr[n] = '\0';
        return EXR_FUN(attr_string_init_static_with_length)( f, attrdata, strptr, attrsz );
    }
    return n < 0 ? -n : 0;
}

/**************************************/

static int extract_attr_string_vector( EXR_TYPE(FILE) *f,
                                       EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                       EXR_TYPE(attr_string_vector) *attrdata,
                                       const char *aname,
                                       const char *tname,
                                       int32_t attrsz )
{
    int rv;
    int32_t nstr, nalloced, nlen, pulled = 0;
    EXR_TYPE(attr_string) *nlist, *clist, nil = {0};
    int32_t n = check_bad_attrsz( f, attrsz, 1, aname, tname );

    if ( n < 0 )
        return -n;

    nstr = 0;
    nalloced = 0;
    clist = NULL;
    while ( pulled < attrsz )
    {
        nlen = 0;
        rv = scratch->sequential_read( scratch, &nlen, sizeof(int32_t) );
        if ( rv != 0 )
        {
            EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_READ_IO),
                "Attribute '%s': Unable to read string length",
                aname );
            goto extract_string_vector_fail;
        }

        pulled += sizeof(int32_t);
        nlen = (int32_t)one_to_native32( (uint32_t)nlen );
        if ( nlen < 0 )
        {
            rv = EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_INVALID_ATTR),
                "Attribute '%s': Invalid size (%d) encountered parsing string vector",
                aname, nlen );
            goto extract_string_vector_fail;
        }

        if ( nalloced == 0 )
        {
            clist = priv_alloc( 4 * sizeof(EXR_TYPE(attr_string)) );
            if ( clist == NULL )
            {
                rv = EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Attribute '%s': Unable to create channel list of size 4", aname );
                goto extract_string_vector_fail;
            }
            nalloced = 4;
        }
        if ( (nstr + 1) >= nalloced )
        {
            nalloced *= 2;
            nlist = priv_alloc( nalloced * sizeof(EXR_TYPE(attr_string)) );
            if ( nlist == NULL )
            {
                rv = EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_OUT_OF_MEMORY),
                    "Attribute '%s': Unable to create memory for string list of size (%d)", aname, nalloced );
                goto extract_string_vector_fail;
            }
            for ( int32_t i = 0; i < nstr; ++i )
                *(nlist + i) = clist[i];
            priv_free( clist );
            clist = nlist;
        }
        nlist = clist + nstr;
        *nlist = nil;
        nstr += 1;
        rv = EXR_FUN(attr_string_init)( f, nlist, nlen );
        if ( rv != 0 )
            goto extract_string_vector_fail;

        if ( scratch->sequential_read( scratch, (void *)nlist->str, (size_t)nlen ) )
        {
            rv = EXR_GETFILE(f)->print_error(
                f, EXR_DEF(ERR_READ_IO),
                "Attribute '%s': Unable to read string of length (%d)",
                aname, nlen );
            goto extract_string_vector_fail;
        }
        *(((char *)nlist->str) + nlen) = '\0';
        pulled += nlen;
    }

    attrdata->n_strings = nstr;
    attrdata->alloc_size = nalloced;
    attrdata->strings = clist;
    return 0;
  extract_string_vector_fail:
    for ( int32_t i = 0; i < nstr; ++i )
        EXR_FUN(attr_string_destroy)( clist + i );
    if ( clist )
        priv_free( clist );
    return rv;
}

/**************************************/

static int extract_attr_tiledesc( EXR_TYPE(FILE) *f,
                                  EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                  EXR_TYPE(attr_tiledesc) *attrdata,
                                  const char *aname,
                                  const char *tname,
                                  int32_t attrsz )
{
    if ( attrsz != (int32_t)sizeof(*attrdata) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp '%s' size %d)",
            aname, attrsz, tname, (int32_t)sizeof(*attrdata) );

    if ( scratch->sequential_read( scratch, attrdata, sizeof(*attrdata) ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Unable to read '%s' %s data",
            aname, tname );

    attrdata->x_size = one_to_native32( attrdata->x_size );
    attrdata->y_size = one_to_native32( attrdata->y_size );

    if ( (int)EXR_GET_TILE_LEVEL_MODE(*attrdata) >= (int)EXR_DEF(TILE_LAST_TYPE) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Attribute '%s': Invalid tile level specification encountered: found enum %d",
            aname, (int)EXR_GET_TILE_LEVEL_MODE(*attrdata) );
    if ( (int)EXR_GET_TILE_ROUND_MODE(*attrdata) >= (int)EXR_DEF(TILE_ROUND_LAST_TYPE) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Attribute '%s': Invalid tile rounding specification encountered: found enum %d",
            aname, (int)EXR_GET_TILE_ROUND_MODE(*attrdata) );
    
    return 0;
}

/**************************************/

static int extract_attr_opaque( EXR_TYPE(FILE) *f,
                                EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                EXR_TYPE(attr_opaquedata) *attrdata,
                                const char *aname,
                                const char *tname,
                                int32_t attrsz )
{
    int32_t n = check_bad_attrsz( f, attrsz, 1, aname, tname );

    if ( n <= 0 )
        return -n;

    if ( EXR_FUN(attr_opaquedata_init)( f, attrdata, (size_t)attrsz ) )
        return 1;

    if ( scratch->sequential_read( scratch, (void *)attrdata->packed_data, (size_t)attrsz ) )
    {
        EXR_FUN(attr_opaquedata_destroy)( attrdata );
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read opaque %s data (%d bytes)",
            aname, tname, attrsz );
    }
    return 0;
}

/**************************************/

static int extract_attr_preview( EXR_TYPE(FILE) *f,
                                 EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                                 EXR_TYPE(attr_preview) *attrdata,
                                 const char *aname,
                                 const char *tname,
                                 int32_t attrsz )
{
    size_t bytes;
    uint32_t sz[2];
    ssize_t fsize = EXR_GETFILE(f)->file_size;

    if ( attrsz < 8 )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s': Invalid size %d (exp '%s' size >= 8)",
            aname, attrsz, tname );

    if ( scratch->sequential_read( scratch, sz, sizeof(uint32_t)*2 ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read preview sizes", aname );

    sz[0] = one_to_native32( sz[0] );
    sz[1] = one_to_native32( sz[1] );
    bytes = 4 * sz[0] * sz[1];
    if ( (size_t)attrsz != (8 + bytes) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Attribute '%s': Invalid size %d (exp '%s' %u x %u * 4 + sizevals)",
            aname, attrsz, tname, sz[0], sz[1] );

    if ( fsize > 0 && bytes >= fsize )
    {
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Attribute '%s', type '%s': Invalid size for preview %u x %u",
            aname, tname, sz[0], sz[1] );
    }

    if ( EXR_FUN(attr_preview_init)( f, attrdata, sz[0], sz[1] ) )
        return 1;

    if ( scratch->sequential_read( scratch, (void *)attrdata->rgba, sz[0] * sz[1] * 4 ) )
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read preview data (%d bytes)", aname, attrsz );

    return 0;
}

/**************************************/

static int check_populate_channels(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    EXR_TYPE(attr_chlist) tmpchans = {0};
    int rv;

    if ( curpart->channels )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'channels' encountered" );

    if ( 0 != strcmp( tname, "chlist" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute 'channels': Invalid type '%s'",
            tname );

    rv = extract_attr_chlist( f, scratch, &(tmpchans), EXR_REQ_CHANNELS_STR, tname, attrsz );
    if ( rv != 0 )
        return -1;

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_CHANNELS_STR,
        EXR_DEF(ATTR_CHLIST), 0, NULL, &(curpart->channels) );
    if ( rv != 0 )
    {
        EXR_FUN(attr_chlist_destroy)( &tmpchans );
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_CHANNELS_STR );
    }

    *(curpart->channels->chlist) = tmpchans;
    return 0;
}

/**************************************/

static int check_populate_compression(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    uint8_t data;
    int rv;

    if ( curpart->compression )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_COMP_STR );

    if ( 0 != strcmp( tname, EXR_REQ_COMP_STR ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_COMP_STR, tname );

    rv = extract_attr_uint8( f, scratch, &data, EXR_REQ_COMP_STR, tname, attrsz,
                             (uint8_t)EXR_DEF(COMPRESSION_LAST_TYPE) );
    if ( rv != 0 )
        return -1;

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_COMP_STR,
        EXR_DEF(ATTR_COMPRESSION), 0, NULL, &(curpart->compression) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_COMP_STR );

    curpart->compression->uc = data;
    curpart->comp_type = (EXR_TYPE(COMPRESSION_TYPE))data;
    return 0;
}

/**************************************/

static int check_populate_dataWindow(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    EXR_TYPE(attr_box2i) tmpdata = {0};
    int rv;

    if ( curpart->dataWindow )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_DATA_STR );

    if ( 0 != strcmp( tname, "box2i" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_DATA_STR, tname );

    rv = extract_attr_32bit( f, scratch, &(tmpdata), EXR_REQ_DATA_STR, tname, attrsz, 4 );
    if ( rv != 0 )
        return -1;

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_DATA_STR,
        EXR_DEF(ATTR_BOX2I), 0, NULL, &(curpart->dataWindow) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'box2i'", EXR_REQ_DATA_STR );

    *(curpart->dataWindow->box2i) = tmpdata;
    curpart->data_window = tmpdata;
    return 0;
}

/**************************************/

static int check_populate_displayWindow(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    EXR_TYPE(attr_box2i) tmpdata = {0};
    int rv;

    if ( curpart->displayWindow )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_DISP_STR );

    if ( 0 != strcmp( tname, "box2i" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_DISP_STR, tname );

    rv = extract_attr_32bit( f, scratch, &(tmpdata), EXR_REQ_DISP_STR, tname, attrsz, 4 );
    if ( rv != 0 )
        return -1;

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_DISP_STR,
        EXR_DEF(ATTR_BOX2I), 0, NULL, &(curpart->displayWindow) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'box2i'", EXR_REQ_DISP_STR );

    *(curpart->displayWindow->box2i) = tmpdata;
    curpart->display_window = tmpdata;
    return 0;
}

/**************************************/

static int check_populate_lineOrder(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    uint8_t data;
    int rv;

    if ( curpart->lineOrder )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_LO_STR );

    if ( 0 != strcmp( tname, EXR_REQ_LO_STR ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_LO_STR, tname );

    rv = extract_attr_uint8( f, scratch, &data, EXR_REQ_LO_STR, tname, attrsz,
                             (uint8_t)EXR_DEF(LINEORDER_LAST_TYPE) );
    if ( rv != 0 )
        return -1;

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_LO_STR,
        EXR_DEF(ATTR_LINEORDER), 0, NULL, &(curpart->lineOrder) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_LO_STR );

    curpart->lineOrder->uc = data;
    curpart->lineorder = data;
    return 0;
}

/**************************************/

static int check_populate_pixelAspectRatio(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;
    union 
    {
        uint32_t ival;
        float fval;
    } tpun;

    if ( curpart->pixelAspectRatio )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_PAR_STR );

    if ( 0 != strcmp( tname, "float" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_PAR_STR, tname );

    if ( attrsz != sizeof(float) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Required attribute '%s': Invalid size %d (exp 4)", EXR_REQ_PAR_STR, attrsz );

    if ( scratch->sequential_read( scratch, &(tpun.ival), sizeof(uint32_t) ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read data (%d bytes)", EXR_REQ_PAR_STR, attrsz );

    tpun.ival = one_to_native32( tpun.ival );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_PAR_STR,
        EXR_DEF(ATTR_FLOAT), 0, NULL, &(curpart->pixelAspectRatio) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_PAR_STR );

    curpart->pixelAspectRatio->f = tpun.fval;
    return 0;
}

/**************************************/

static int check_populate_screenWindowCenter(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;
    EXR_TYPE(attr_v2f) tmpdata;

    if ( curpart->screenWindowCenter )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_SCR_WC_STR );

    if ( 0 != strcmp( tname, "v2f" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_SCR_WC_STR, tname );

    if ( attrsz != sizeof(EXR_TYPE(attr_v2f)) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Required attribute '%s': Invalid size %d (exp %lu)",
            EXR_REQ_SCR_WC_STR, attrsz, sizeof(EXR_TYPE(attr_v2f)) );

    if ( scratch->sequential_read( scratch, &tmpdata, sizeof(EXR_TYPE(attr_v2f)) ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read data (%d bytes)", EXR_REQ_SCR_WC_STR, attrsz );

    priv_to_native32( &tmpdata, 2 );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_SCR_WC_STR,
        EXR_DEF(ATTR_V2F), 0, NULL, &(curpart->screenWindowCenter) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_SCR_WC_STR );
    return 0;
}

/**************************************/

static int check_populate_screenWindowWidth(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;
    union 
    {
        uint32_t ival;
        float fval;
    } tpun;

    if ( curpart->screenWindowWidth )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute '%s' encountered", EXR_REQ_SCR_WW_STR );

    if ( 0 != strcmp( tname, "float" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute '%s': Invalid type '%s'", EXR_REQ_SCR_WW_STR, tname );

    if ( attrsz != sizeof(float) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_SIZE_MISMATCH),
            "Required attribute '%s': Invalid size %d (exp 4)", EXR_REQ_SCR_WW_STR, attrsz );

    if ( scratch->sequential_read( scratch, &(tpun.ival), sizeof(uint32_t) ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_READ_IO),
            "Attribute '%s': Unable to read data (%d bytes)", EXR_REQ_SCR_WW_STR, attrsz );

    tpun.ival = one_to_native32( tpun.ival );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_SCR_WW_STR,
        EXR_DEF(ATTR_FLOAT), 0, NULL, &(curpart->screenWindowWidth) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_SCR_WW_STR );

    curpart->screenWindowWidth->f = tpun.fval;
    return 0;
}

/**************************************/

static int check_populate_tiles(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;
    EXR_TYPE(attr_tiledesc) tmpdata = {0};

    if ( curpart->tiles )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'tiles' encountered" );

    if ( 0 != strcmp( tname, "tiledesc" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute 'tiles': Invalid type '%s'",
            tname );

    if ( attrsz != sizeof(EXR_TYPE(attr_tiledesc)) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute 'tiles': Invalid size %d (exp %lu)",
            attrsz, sizeof(EXR_TYPE(attr_tiledesc)) );

    rv = scratch->sequential_read( scratch, &tmpdata, sizeof(tmpdata) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->report_error(
            f, rv, "Unable to read 'tiles' data" );

    tmpdata.x_size = one_to_native32( tmpdata.x_size );
    tmpdata.y_size = one_to_native32( tmpdata.y_size );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_TILES_STR, EXR_DEF(ATTR_TILEDESC),
        0, NULL, &(curpart->tiles) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'tiledesc'", EXR_REQ_TILES_STR );

    *(curpart->tiles->tiledesc) = tmpdata;
    return 0;
}

/**************************************/

static int check_populate_name(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;
    uint8_t *outstr;
    int32_t n = check_bad_attrsz( f, attrsz, 1, EXR_REQ_NAME_STR, tname );

    if ( n < 0 )
        return -n;

    if ( curpart->name )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'name' encountered" );

    if ( 0 != strcmp( tname, "string" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "attribute 'name': Invalid type '%s'", tname );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_NAME_STR,
        EXR_DEF(ATTR_STRING), attrsz + 1, &outstr, &(curpart->name) );
    if ( rv != 0 )
    {
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'string'", EXR_REQ_NAME_STR );
    }

    rv = scratch->sequential_read( scratch, outstr, attrsz );
    if ( rv != 0 )
    {
        EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->name );
        curpart->name = NULL;
        return - EXR_GETFILE(f)->report_error(
            f, rv, "Unable to read 'name' data" );
    }
    outstr[attrsz] = '\0';

    rv = EXR_FUN(attr_string_init_static_with_length)( f, curpart->name->string, (const char *)outstr, attrsz );
    if ( rv != 0 )
    {
        EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->name );
        curpart->name = NULL;
        return - EXR_GETFILE(f)->report_error(
            f, rv, "Unable to read 'name' data" );
    }

    return 0;
}

/**************************************/

static int check_populate_type(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
    int rv;
    uint8_t *outstr;
    int32_t n = check_bad_attrsz( f, attrsz, 1, EXR_REQ_TYPE_STR, tname );

    if ( n < 0 )
        return n;

    if ( curpart->type )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'type' encountered" );

    if ( 0 != strcmp( tname, "string" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "Required attribute 'type': Invalid type '%s'", tname );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_TYPE_STR,
        EXR_DEF(ATTR_STRING), attrsz + 1, &outstr, &(curpart->type) );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'string'", EXR_REQ_TYPE_STR );
    }

    rv = scratch->sequential_read( scratch, outstr, attrsz );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->type );
        curpart->type = NULL;
        return - EXR_GETFILE(f)->report_error(
            f, rv, "Unable to read 'name' data" );
    }
    outstr[attrsz] = '\0';

    rv = EXR_FUN(attr_string_init_static_with_length)( f, curpart->type->string, (const char *)outstr, attrsz );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->type );
        curpart->type = NULL;
        return - EXR_GETFILE(f)->report_error(
            f, rv, "Unable to read 'name' data" );
    }

    if ( strcmp( (const char *)outstr, "scanlineimage" ) == 0 )
        curpart->storage_mode = EXR_DEF(STORAGE_SCANLINE);
    else if ( strcmp( (const char *)outstr, "tiledimage" ) == 0 )
        curpart->storage_mode = EXR_DEF(STORAGE_TILED);
    else if ( strcmp( (const char *)outstr, "deepscanline" ) == 0 )
        curpart->storage_mode = EXR_DEF(STORAGE_DEEP_SCANLINE);
    else if ( strcmp( (const char *)outstr, "deeptile" ) == 0 )
        curpart->storage_mode = EXR_DEF(STORAGE_DEEP_TILED);
    else
    {
        EXR_FUN(attr_list_remove)( f, &(curpart->attributes), curpart->type );
        curpart->type = NULL;
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "attribute 'type': Invalid type string '%s'", outstr );
    }

    return 0;
}

/**************************************/

static int check_populate_version(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;

    if ( curpart->version )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'version' encountered" );

    if ( 0 != strcmp( tname, "int" ) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "attribute 'version': Invalid type '%s'", tname );

    if ( attrsz != sizeof(int32_t) )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "attribute 'version': Invalid size %d (exp 4)", attrsz );

    if ( scratch->sequential_read( scratch, &attrsz, sizeof(int32_t) ) )
        return - EXR_GETFILE(f)->report_error(
            f, EXR_DEF(ERR_READ_IO), "Unable to read chunkCount data" );

    attrsz = (int32_t)one_to_native32( (uint32_t)attrsz );
    if ( attrsz != 1 )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Invalid version %d: expect 1", attrsz );

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_VERSION_STR,
        EXR_DEF(ATTR_INT), 0, NULL, &(curpart->version) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_VERSION_STR );
    curpart->version->i = attrsz;
    return 0;
}

/**************************************/

static int check_populate_chunk_count(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
    const char *tname,
    int32_t attrsz )
{
    int rv;

    if ( curpart->chunkCount )
        return - EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Duplicate copy of required attribute 'chunkCount' encountered" );

    if ( 0 != strcmp( tname, "int" ) )
    {
        EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_TYPE_MISMATCH),
            "attribute 'chunkCount': Invalid type '%s'", tname );
        return -1;
    }
    if ( attrsz != sizeof(int32_t) )
    {
        EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_INVALID_ATTR),
            "Required attribute 'chunkCount': Invalid size %d (exp 4)", attrsz );
        return -1;
    }

    if ( scratch->sequential_read( scratch, &attrsz, sizeof(int32_t) ) )
    {
        EXR_GETFILE(f)->report_error( f, EXR_DEF(ERR_READ_IO), "Unable to read chunkCount data" );
        return -1;
    }

    rv = EXR_FUN(attr_list_add_static_name)(
        f, &(curpart->attributes), EXR_REQ_CHUNK_COUNT_STR,
        EXR_DEF(ATTR_INT), 0, NULL, &(curpart->chunkCount) );
    if ( rv != 0 )
        return - EXR_GETFILE(f)->print_error(
            f, rv, "Unable initialize attribute '%s', type 'int'", EXR_REQ_CHUNK_COUNT_STR );

    attrsz = (int32_t)one_to_native32( (uint32_t)attrsz );
    curpart->chunkCount->i = attrsz;
    curpart->chunk_count = attrsz;
    return 0;
}

/**************************************/

static int check_req_attr( EXR_TYPE(FILE) *file,
                           EXR_TYPE(PRIV_PART) *curpart,
                           EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch,
                           const char *aname,
                           const char *tname,
                           int32_t attrsz )
{
    switch ( aname[0] )
    {
        case 'c':
            if ( 0 == strcmp( aname, EXR_REQ_CHANNELS_STR ) )
                return check_populate_channels( file, curpart, scratch, tname, attrsz );
            if ( 0 == strcmp( aname, EXR_REQ_COMP_STR ) )
                return check_populate_compression( file, curpart, scratch, tname, attrsz );
            if ( 0 == strcmp( aname, EXR_REQ_CHUNK_COUNT_STR ) )
                return check_populate_chunk_count( file, curpart, scratch, tname, attrsz );
            break;
        case 'd':
            if ( 0 == strcmp( aname, EXR_REQ_DATA_STR ) )
                return check_populate_dataWindow( file, curpart, scratch, tname, attrsz );
            if ( 0 == strcmp( aname, EXR_REQ_DISP_STR ) )
                return check_populate_displayWindow( file, curpart, scratch, tname, attrsz );
            break;
        case 'l':
            if ( 0 == strcmp( aname, EXR_REQ_LO_STR ) )
                return check_populate_lineOrder( file, curpart, scratch, tname, attrsz );
            break;
        case 'n':
            if ( 0 == strcmp( aname, EXR_REQ_NAME_STR ) )
                return check_populate_name( file, curpart, scratch, tname, attrsz );
            break;
        case 'p':
            if ( 0 == strcmp( aname, EXR_REQ_PAR_STR ) )
                return check_populate_pixelAspectRatio( file, curpart, scratch, tname, attrsz );
            break;
        case 's':
            if ( 0 == strcmp( aname, EXR_REQ_SCR_WC_STR ) )
                return check_populate_screenWindowCenter( file, curpart, scratch, tname, attrsz );
            if ( 0 == strcmp( aname, EXR_REQ_SCR_WW_STR ) )
                return check_populate_screenWindowWidth( file, curpart, scratch, tname, attrsz );
            break;
        case 't':
            if ( 0 == strcmp( aname, EXR_REQ_TILES_STR ) )
                return check_populate_tiles( file, curpart, scratch, tname, attrsz );
            if ( 0 == strcmp( aname, EXR_REQ_TYPE_STR ) )
                return check_populate_type( file, curpart, scratch, tname, attrsz );
        case 'v':
            if ( 0 == strcmp( aname, EXR_REQ_VERSION_STR ) )
                return check_populate_version( file, curpart, scratch, tname, attrsz );
            break;
        default:
            break;
    }

    return 1;
}

/**************************************/

static int pull_attr(
    EXR_TYPE(FILE) *f,
    EXR_TYPE(PRIV_PART) *curpart,
    uint8_t init_byte,
    EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch )
{
    char name[256], type[256];
    int rv;
    int32_t namelen = 0, typelen = 0;
    int32_t attrsz = 0;
    const int32_t maxlen = EXR_GETFILE(f)->max_name_length;

    if ( init_byte > 0 && ( init_byte > 126 || (init_byte < ' ' && init_byte != '\t' ) ) )
    {
        namelen = 0;
        //return EXR_GETFILE(f)->print_error(
        //    f, EXR_DEF(ERR_FILE_BAD_HEADER),
        //    "Invalid non-printable character %d (0x%02X) encountered parsing text", (int)init_byte, (int)init_byte );
    }
    else
    {
        name[0] = (char)init_byte;
        namelen = 1;
    }
    if ( read_text( f, name, &namelen, maxlen, scratch, "attribute name" ) )
        return 1;
    if ( read_text( f, type, &typelen, maxlen, scratch, "attribute type" ) )
        return 1;
    if ( namelen == 0 )
    {
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_FILE_BAD_HEADER),
            "Invalid empty string encountered parsing attribute name" );
    }
    if ( typelen == 0 )
    {
        return EXR_GETFILE(f)->print_error(
            f, EXR_DEF(ERR_FILE_BAD_HEADER),
            "Invalid empty string encountered parsing attribute type for attribute '%s'", name );
    }

    rv = scratch->sequential_read( scratch, &attrsz, sizeof(int32_t) );
    if ( rv != 0 )
        return EXR_GETFILE(f)->print_error(
            f, rv,
            "Unable to read attribute size for attribute '%s', type '%s'",
            name, type );
    attrsz = one_to_native32( attrsz );

    rv = check_req_attr( f, curpart, scratch, name, type, attrsz );
    /* error check */
    if ( rv < 0 )
        return 1;

    /* not a required attr, just a normal one */
    if ( rv > 0 )
    {
        EXR_TYPE(attribute) *nattr = NULL;
        uint8_t *strptr;
        if ( ! strcmp( type, "string" ) )
        {
            rv = EXR_FUN(attr_list_add_by_type)( f, &(curpart->attributes), name, type,
                                                 attrsz + 1, &strptr, &nattr );
        }
        else
        {
            rv = EXR_FUN(attr_list_add_by_type)( f, &(curpart->attributes), name, type,
                                                 0, NULL, &nattr );
        }
        if ( rv != EXR_DEF(ERR_SUCCESS) )
            return EXR_GETFILE(f)->print_error(
                f, rv, "Unable initialize attribute '%s', type '%s'", name, type );

        switch ( nattr->type )
        {
            case EXR_DEF(ATTR_BOX2I):
                rv = extract_attr_32bit( f, scratch, nattr->box2i, name, type, attrsz, 4 );
                break;
            case EXR_DEF(ATTR_BOX2F):
                rv = extract_attr_32bit( f, scratch, nattr->box2f, name, type, attrsz, 4 );
                break;
            case EXR_DEF(ATTR_CHLIST):
                rv = extract_attr_chlist( f, scratch, nattr->chlist, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_CHROMATICITIES):
                rv = extract_attr_32bit( f, scratch, nattr->chromaticities, name, type, attrsz, 8 );
                break;
            case EXR_DEF(ATTR_COMPRESSION):
                rv = extract_attr_uint8( f, scratch, &(nattr->uc), name, type, attrsz,
                                         (uint8_t)EXR_DEF(COMPRESSION_LAST_TYPE) );
                break;
            case EXR_DEF(ATTR_ENVMAP):
                rv = extract_attr_uint8( f, scratch, &(nattr->uc), name, type, attrsz,
                                         (uint8_t)EXR_DEF(ENVMAP_LAST_TYPE) );
                break;
            case EXR_DEF(ATTR_LINEORDER):
                rv = extract_attr_uint8( f, scratch, &(nattr->uc), name, type, attrsz,
                                         (uint8_t)EXR_DEF(LINEORDER_LAST_TYPE) );
                break;
            case EXR_DEF(ATTR_DOUBLE):
                rv = extract_attr_64bit( f, scratch, &(nattr->d), name, type, attrsz, 1 );
                break;
            case EXR_DEF(ATTR_FLOAT):
                rv = extract_attr_32bit( f, scratch, &(nattr->f), name, type, attrsz, 1 );
                break;
            case EXR_DEF(ATTR_FLOAT_VECTOR):
                rv = extract_attr_float_vector( f, scratch, nattr->floatvector, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_INT):
                rv = extract_attr_32bit( f, scratch, &(nattr->i), name, type, attrsz, 1 );
                break;
            case EXR_DEF(ATTR_KEYCODE):
                rv = extract_attr_32bit( f, scratch, nattr->keycode, name, type, attrsz, 7 );
                break;
            case EXR_DEF(ATTR_M33F):
                rv = extract_attr_32bit( f, scratch, nattr->m33f->m, name, type, attrsz, 9 );
                break;
            case EXR_DEF(ATTR_M33D):
                rv = extract_attr_64bit( f, scratch, nattr->m33d->m, name, type, attrsz, 9 );
                break;
            case EXR_DEF(ATTR_M44F):
                rv = extract_attr_32bit( f, scratch, nattr->m44f->m, name, type, attrsz, 16 );
                break;
            case EXR_DEF(ATTR_M44D):
                rv = extract_attr_64bit( f, scratch, nattr->m44d->m, name, type, attrsz, 16 );
                break;
            case EXR_DEF(ATTR_PREVIEW):
                rv = extract_attr_preview( f, scratch, nattr->preview, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_RATIONAL):
                rv = extract_attr_32bit( f, scratch, nattr->rational, name, type, attrsz, 2 );
                break;
            case EXR_DEF(ATTR_STRING):
                rv = extract_attr_string( f, scratch, nattr->string, name, type, attrsz, (char *)strptr );
                break;
            case EXR_DEF(ATTR_STRING_VECTOR):
                rv = extract_attr_string_vector( f, scratch, nattr->stringvector, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_TILEDESC):
                rv = extract_attr_tiledesc( f, scratch, nattr->tiledesc, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_TIMECODE):
                rv = extract_attr_32bit( f, scratch, nattr->timecode, name, type, attrsz, 2 );
                break;
            case EXR_DEF(ATTR_V2I):
                rv = extract_attr_32bit( f, scratch, nattr->v2i->arr, name, type, attrsz, 2 );
                break;
            case EXR_DEF(ATTR_V2F):
                rv = extract_attr_32bit( f, scratch, nattr->v2f->arr, name, type, attrsz, 2 );
                break;
            case EXR_DEF(ATTR_V2D):
                rv = extract_attr_64bit( f, scratch, nattr->v2d->arr, name, type, attrsz, 2 );
                break;
            case EXR_DEF(ATTR_V3I):
                rv = extract_attr_32bit( f, scratch, nattr->v3i->arr, name, type, attrsz, 3 );
                break;
            case EXR_DEF(ATTR_V3F):
                rv = extract_attr_32bit( f, scratch, nattr->v3f->arr, name, type, attrsz, 3 );
                break;
            case EXR_DEF(ATTR_V3D):
                rv = extract_attr_64bit( f, scratch, nattr->v3d->arr, name, type, attrsz, 3 );
                break;
            case EXR_DEF(ATTR_OPAQUE):
                rv = extract_attr_opaque( f, scratch, nattr->opaque, name, type, attrsz );
                break;
            case EXR_DEF(ATTR_UNKNOWN):
            case EXR_DEF(ATTR_LAST_KNOWN_TYPE):
            default:
                rv = EXR_GETFILE(f)->print_error(
                    f, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid type '%s' for attribute '%s'", type, name );
                break;
        }
        if ( rv != 0 )
        {
            EXR_FUN(attr_list_remove)( f, &(curpart->attributes), nattr );
        }
    }
    
    return rv;
}

/**************************************/

/*  floor( log(x) / log(2) ) */
static int32_t floor_log2( int64_t x )
{
    int32_t y = 0;
    while ( x > 1 )
    {
        y += 1;
        x >>= 1;
    }
    return y;
}

/**************************************/

/*  ceil( log(x) / log(2) ) */
static int32_t ceil_log2( int64_t x )
{
    int32_t y = 0, r = 0;
    while ( x > 1 )
    {
        if ( x & 1 )
            r = 1;
        y += 1;
        x >>= 1;
    }
    return y + r;
}

/**************************************/

static int64_t calc_level_size( int mind, int maxd, int level,
                                EXR_TYPE(TILE_ROUND_MODE) rounding )
{
    int64_t dsize = (int64_t)maxd - (int64_t)mind + 1;
    int b = ( 1 << level );
    int64_t retsize = dsize / b;

    if ( rounding == EXR_DEF(TILE_ROUND_UP) && retsize * b < dsize )
        retsize += 1;

    if ( retsize < 1 )
        retsize = 1;
    return retsize;
}

/**************************************/

static int compute_tile_information( EXR_TYPE(FILE) *f, EXR_TYPE(PRIV_PART) *curpart )
{
    int rv = 0;
    if ( curpart->storage_mode == EXR_DEF(STORAGE_SCANLINE) ||
         curpart->storage_mode == EXR_DEF(STORAGE_DEEP_SCANLINE) )
        return EXR_DEF(ERR_SUCCESS);

    if ( ! curpart->tiles )
        return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_INVALID_ARGUMENT) );

    if ( curpart->tile_level_tile_count_x == NULL )
    {
        const EXR_TYPE(attr_box2i) dw = curpart->data_window;
        const EXR_TYPE(attr_tiledesc) *tiledesc = curpart->tiles->tiledesc;
        int64_t w, h;
        int32_t numX, numY;
        int32_t *levcntX = NULL;
        int32_t *levcntY = NULL;
        int32_t *levszX = NULL;
        int32_t *levszY = NULL;

        w = dw.x_max - dw.x_min + 1;
        h = dw.y_max - dw.y_min + 1;

        switch ( EXR_GET_TILE_LEVEL_MODE( (*tiledesc) ) )
        {
            case EXR_DEF(TILE_ONE_LEVEL):
                numX = numY = 1;
                break;
            case EXR_DEF(TILE_MIPMAP_LEVELS):
                if ( EXR_GET_TILE_ROUND_MODE( (*tiledesc) ) == EXR_DEF(TILE_ROUND_DOWN) )
                {
                    numX = floor_log2( w > h ? w : h ) + 1;
                    numY = numX;
                }
                else
                {
                    numX = ceil_log2( w > h ? w : h ) + 1;
                    numY = numX;
                }
                break;
            case EXR_DEF(TILE_RIPMAP_LEVELS):
                if ( EXR_GET_TILE_ROUND_MODE( (*tiledesc) ) == EXR_DEF(TILE_ROUND_DOWN) )
                {
                    numX = floor_log2( w ) + 1;
                    numY = floor_log2( h ) + 1;
                }
                else
                {
                    numX = ceil_log2( w ) + 1;
                    numY = ceil_log2( h ) + 1;
                }
                break;
            default:
                return -1;
        }

        curpart->num_tile_levels_x = numX;
        curpart->num_tile_levels_y = numY;
        levcntX = (int32_t *)priv_alloc( 2 * ( numX + numY ) * sizeof(int32_t) );
        if ( levcntX == NULL )
            return EXR_GETFILE(f)->standard_error( f, EXR_DEF(ERR_OUT_OF_MEMORY) );
        levszX = levcntX + numX;
        levcntY = levszX + numX;
        levszY = levcntY + numY;

        for ( int l = 0; l < numX; ++l )
        {
            int64_t sx = calc_level_size( dw.x_min, dw.x_max, l,
                                          EXR_GET_TILE_ROUND_MODE( (*tiledesc) ) );
            levcntX[l] = (int32_t)( ( (uint64_t)sx + tiledesc->x_size - 1 ) / tiledesc->x_size );
            levszX[l] = sx;
        }

        for ( int l = 0; l < numY; ++l )
        {
            int64_t sy = calc_level_size( dw.y_min, dw.y_max, l,
                                          EXR_GET_TILE_ROUND_MODE( (*tiledesc) ) );
            levcntY[l] = (int32_t)( ( (uint64_t)sy + tiledesc->y_size - 1 ) / tiledesc->y_size );
            levszY[l] = sy;
        }

        curpart->tile_level_tile_count_x = levcntX;
        curpart->tile_level_tile_count_y = levcntY;
        curpart->tile_level_tile_size_x = levszX;
        curpart->tile_level_tile_size_y = levszY;
    }
    return rv;
}

/**************************************/

static int32_t compute_chunk_offset_size( EXR_TYPE(PRIV_PART) *curpart )
{
    int32_t retval = 0;
    const EXR_TYPE(attr_box2i) dw = curpart->data_window;
    const EXR_TYPE(attr_chlist) *channels = curpart->channels->chlist;
    uint64_t unpackedsize = 0;
    int64_t w;

    w = (int64_t)dw.x_max - (int64_t)dw.x_min + 1;

    if ( curpart->tiles )
    {
        const EXR_TYPE(attr_tiledesc) *tiledesc = curpart->tiles->tiledesc;
        int64_t tilecount = 0;

        switch ( EXR_GET_TILE_LEVEL_MODE( (*tiledesc) ) )
        {
            case EXR_DEF(TILE_ONE_LEVEL):
            case EXR_DEF(TILE_MIPMAP_LEVELS):
                for ( int l = 0; l < curpart->num_tile_levels_x; ++l )
                    tilecount += ( (int64_t)curpart->tile_level_tile_count_x[l] *
                                   (int64_t)curpart->tile_level_tile_count_y[l] );
                if ( tilecount > (int64_t)INT_MAX )
                    return -1;
                retval = (int32_t)tilecount;
                break;
            case EXR_DEF(TILE_RIPMAP_LEVELS):
                for ( int lx = 0; lx < curpart->num_tile_levels_x; ++lx )
                {
                    for ( int ly = 0; ly < curpart->num_tile_levels_y; ++ly )
                    {
                        tilecount += ( (int64_t)curpart->tile_level_tile_count_x[lx] *
                                       (int64_t)curpart->tile_level_tile_count_y[ly] );
                    }
                }
                if ( tilecount > (int64_t)INT_MAX )
                    return -1;
                retval = (int32_t)tilecount;
                break;
            default:
                return -1;
        }

        for ( int c = 0; c < channels->num_channels; ++c )
        {
            int32_t xsamp = channels->entries[c].x_sampling;
            int32_t ysamp = channels->entries[c].y_sampling;
            uint64_t cunpsz = 0;
            if ( channels->entries[c].pixel_type == EXR_DEF(PIXEL_HALF) )
                cunpsz = 2;
            else
                cunpsz = 4;
            unpackedsize += ( cunpsz *
                              (uint64_t)( ( tiledesc->x_size + xsamp - 1 ) / xsamp ) *
                              (uint64_t)( ( tiledesc->y_size + ysamp - 1 ) / ysamp ) );
        }
        curpart->unpacked_size_per_chunk = unpackedsize;
    }
    else
    {
        int linePerChunk;
        switch ( curpart->comp_type )
        {
            case EXR_DEF(COMPRESSION_NONE):
            case EXR_DEF(COMPRESSION_RLE):
            case EXR_DEF(COMPRESSION_ZIPS):
                linePerChunk = 1;
                break;
            case EXR_DEF(COMPRESSION_ZIP):
            case EXR_DEF(COMPRESSION_PXR24):
                linePerChunk = 16;
                break;
            case EXR_DEF(COMPRESSION_PIZ):
            case EXR_DEF(COMPRESSION_B44):
            case EXR_DEF(COMPRESSION_B44A):
            case EXR_DEF(COMPRESSION_DWAA):
                linePerChunk = 32;
                break;
            case EXR_DEF(COMPRESSION_DWAB):
                linePerChunk = 256;
                break;
            default:
                /* ERROR CONDITION */
                return -1;
        }

        curpart->lines_per_chunk = linePerChunk;
        for ( int c = 0; c < channels->num_channels; ++c )
        {
            int32_t xsamp = channels->entries[c].x_sampling;
            int32_t ysamp = channels->entries[c].y_sampling;
            uint64_t cunpsz = 0;
            if ( channels->entries[c].pixel_type == EXR_DEF(PIXEL_HALF) )
                cunpsz = 2;
            else
                cunpsz = 4;
            unpackedsize += ( cunpsz *
                              (uint64_t)( ( w + xsamp - 1 ) / xsamp ) *
                              (uint64_t)( ( linePerChunk + ysamp - 1 ) / ysamp ) );
        }
        curpart->unpacked_size_per_chunk = unpackedsize;
        
        /* h = max - min + 1, but to do size / divide by round,
         * we'd do linePerChunk - 1, so the math cancels */
        retval = ( dw.y_max - dw.y_min + linePerChunk ) / linePerChunk;
    }
    return retval;
}

/**************************************/

static int update_chunk_offsets( EXR_TYPE(PRIV_FILE) *file, EXR_TYPE(PRIV_SEQ_SCRATCH) *scratch )
{
    EXR_TYPE(PRIV_PART) *curpart, *prevpart;
    int rv = EXR_DEF(ERR_SUCCESS);

    if ( ! file->parts )
        return EXR_DEF(ERR_INVALID_ARGUMENT);

    file->parts[0]->chunk_table_offset = scratch->fileoff - scratch->navail;
    prevpart = file->parts[0];
    for ( int p = 0; p < file->num_parts; ++p )
    {
        curpart = file->parts[p];

        rv = compute_tile_information( (EXR_TYPE(FILE) *)file, curpart );
        if ( rv != 0 )
            break;

        int32_t ccount = compute_chunk_offset_size( curpart );
        if ( ccount < 0 )
        {
            rv = file->print_error(
                file, EXR_DEF(ERR_INVALID_ATTR),
                "Invalid chunk count (%d) for part '%s'",
                ccount,
                (curpart->name ? curpart->name->string->str : "<first>") );
            break;
        }

        if ( curpart->chunk_count < 0 )
            curpart->chunk_count = ccount;
        else if ( curpart->chunk_count != ccount )
        {
            /* fatal error or just ignore it? c++ seemed to just ignore it entirely, we can at least warn */
            file->print_error(
                file, EXR_DEF(ERR_INVALID_ATTR),
                "Invalid chunk count (%d) for part '%s', expect (%d)",
                curpart->chunk_count,
                (curpart->name ? curpart->name->string->str : "<first>"),
                ccount );
            curpart->chunk_count = ccount;
        }
        if ( prevpart != curpart )
            curpart->chunk_table_offset = prevpart->chunk_table_offset + sizeof(uint64_t) * prevpart->chunk_count;
        prevpart = curpart;
    }
    return rv;
}

/**************************************/

int priv_parse_header( EXR_TYPE(PRIV_FILE) *file )
{
    EXR_TYPE(PRIV_SEQ_SCRATCH) scratch;
    EXR_TYPE(PRIV_PART) *curpart;
    uint32_t magic_and_version[2];
    uint32_t flags;
    uint8_t next_byte;
    int rv = EXR_DEF(ERR_UNKNOWN), got_attr;
    off_t curoffset = 0;

    rv = priv_init_scratch( file, &scratch, 0 );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        priv_destroy_scratch( &scratch );
        return rv;
    }

    rv = scratch.sequential_read( &scratch, magic_and_version, sizeof(uint32_t) * 2 );
    if ( rv != EXR_DEF(ERR_SUCCESS) )
    {
        file->report_error( file, EXR_DEF(ERR_READ_IO), "Unable to read magic and version flags" );
        priv_destroy_scratch( &scratch );
        return rv;
    }

    priv_to_native32( magic_and_version, 2 );
	if ( magic_and_version[0] != 20000630 )
	{
        rv = file->print_error(
            file, EXR_DEF(ERR_FILE_BAD_HEADER),
            "File is not an OpenEXR file: magic 0x%08X (%d) flags 0x%08X",
            magic_and_version[0], (int)magic_and_version[0], magic_and_version[1] );
        priv_destroy_scratch( &scratch );
        return rv;
    }

    flags = magic_and_version[1];

    file->version = flags & EXR_FILE_VERSION_MASK;
    if ( file->version != 2 )
    {
        rv = file->print_error(
            file, EXR_DEF(ERR_FILE_BAD_HEADER),
            "File is of an unsupported version: %d, magic 0x%08X flags 0x%08X",
            (int)file->version, magic_and_version[0], magic_and_version[1] );
        priv_destroy_scratch( &scratch );
        return rv;
    }

    flags = flags & ~(EXR_FILE_VERSION_MASK);
    if ( ( flags & ~EXR_VALID_FLAGS ) != 0 )
    {
        rv = file->print_error(
            file, EXR_DEF(ERR_FILE_BAD_HEADER),
            "File has an unsupported flags: magic 0x%08X flags 0x%08X",
            magic_and_version[0], magic_and_version[1] );
        priv_destroy_scratch( &scratch );
        return rv;
    }

    curpart = file->parts[0];
    if ( ! curpart )
    {
        rv = file->report_error(
            file, EXR_DEF(ERR_INVALID_ARGUMENT),
            "Error during file initialization" );
        priv_destroy_scratch( &scratch );
        return rv;
    }

    file->is_singlepart_tiled = ( flags & EXR_TILED_FLAG ) ? 1 : 0;
    file->max_name_length = ( flags & EXR_LONG_NAMES_FLAG ) ? EXR_LONGNAME_MAXLEN : EXR_SHORTNAME_MAXLEN;
    file->has_nonimage_data = ( flags & EXR_NON_IMAGE_FLAG ) ? 1 : 0;
    file->is_multipart = ( flags & EXR_MULTI_PART_FLAG ) ? 1 : 0;
    if ( file->is_singlepart_tiled )
    {
        if ( file->has_nonimage_data || file->is_multipart )
        {
            rv = file->print_error(
                file, EXR_DEF(ERR_FILE_BAD_HEADER),
                "Invalid combination of version flags: single part found, but also marked as deep (%d) or multipart (%d)",
                (int)file->has_nonimage_data, (int)file->is_multipart );
            priv_destroy_scratch( &scratch );
            return rv;
        }
        curpart->storage_mode = EXR_DEF(STORAGE_TILED);
    }
    else
        curpart->storage_mode = EXR_DEF(STORAGE_SCANLINE);

    do
    {
        rv = scratch.sequential_read( &scratch, &next_byte, 1 );
        if ( rv != EXR_DEF(ERR_SUCCESS) )
        {
            rv = file->report_error(
                file, EXR_DEF(ERR_FILE_BAD_HEADER),
                "Unable to extract header byte" );
            priv_destroy_scratch( &scratch );
            return rv;
        }

        if ( next_byte == '\0' )
        {
            rv = priv_validate_read_part( file, curpart );
            if ( rv != EXR_DEF(ERR_SUCCESS) )
            {
                priv_destroy_scratch( &scratch );
                return rv;
            }

            if ( ! file->is_multipart )
            {
                /* got a terminal mark, not multipart, so finished */
                break;
            }
            
            rv = scratch.sequential_read( &scratch, &next_byte, 1 );
            if ( rv != EXR_DEF(ERR_SUCCESS) )
            {
                rv = file->report_error(
                    file, EXR_DEF(ERR_FILE_BAD_HEADER),
                    "Unable to go to next part definition" );
                priv_destroy_scratch( &scratch );
                return rv;
            }

            if ( next_byte == '\0' )
            {
                /* got a second terminator, finished with the
                 * headers, can read chunk offsets next */
                break;
            }

            rv = priv_add_part( file, &curpart );
        }

        if ( rv == EXR_DEF(ERR_SUCCESS) )
            rv = pull_attr( file, curpart, next_byte, &scratch );
        if ( rv != EXR_DEF(ERR_SUCCESS) )
            break;
    } while ( 1 );

    if ( rv == EXR_DEF(ERR_SUCCESS) )
        rv = update_chunk_offsets( file, &scratch );

    priv_destroy_scratch( &scratch );
    return rv;
}
