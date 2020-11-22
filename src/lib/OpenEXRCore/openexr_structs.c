/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_priv_structs.h"
#include "openexr_priv_memory.h"
#include "openexr_priv_constants.h"

#include <IlmBaseConfig.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef ILMBASE_THREADING_ENABLED
#  ifdef _WIN32
#    include <synchapi.h>
#  else
#    include <pthread.h>
#  endif
#endif

/**************************************/

static int default_error_cb( EXR_TYPE(FILE) *f, int code, const char *msg )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
#ifdef ILMBASE_THREADING_ENABLED
#  ifdef _WIN32
    static CRITICAL_SECTION sMutex;
    volatile static long initialized = 0;
    if ( InterlockedIncrement( &initialized ) == 1 )
        InitializeCriticalSection( &sMutex );
    initialized = 1; // avoids overflow on long running programs...
#  else
    static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
#  endif
#endif

    if ( file && file->error_cb )
    {
        file->error_cb( file, code, msg );
        return code;
    }

#ifdef ILMBASE_THREADING_ENABLED
#  ifdef _WIN32
    EnterCriticalSection( &sMutex );
#  else
    pthread_mutex_lock( &sMutex );
#  endif
#endif
    if ( file )
    {
        if ( file->filename.str )
            fprintf( stderr, "%s: %s\n", file->filename.str, msg );
        else
            fprintf( stderr, "File 0x%p: %s\n", (void *)file, msg );
    }
    else
        fprintf( stderr, "<ERROR>: %s\n", msg );
    fflush( stderr );

#ifdef ILMBASE_THREADING_ENABLED
#  ifdef _WIN32
    LeaveCriticalSection( &sMutex );
#  else
    pthread_mutex_unlock( &sMutex );
#  endif
#endif
    return code;
}

static int dispatch_error( EXR_TYPE(FILE) *f, int code, const char *msg )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
    if ( file && file->error_cb )
    {
        file->error_cb( file, code, msg );
        return code;
    }

    return default_error_cb( f, code, msg );
}

/**************************************/

static int dispatch_standard_error( EXR_TYPE(FILE) *file, int code )
{
    return dispatch_error( file, code, EXR_FUN(get_default_error_message)( code ) );
}

/**************************************/

static int dispatch_print_error( EXR_TYPE(FILE) *file, int code, const char *msg, ... ) EXR_PRINTF_FUNC_ATTRIBUTE;
static int dispatch_print_error( EXR_TYPE(FILE) *f, int code, const char *msg, ... )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
    char stackbuf[256];
    char *heapbuf = NULL;
    int nwrit = 0;
    va_list fmtargs;

    va_start( fmtargs, msg );
    {
        va_list stkargs;

        va_copy( stkargs, fmtargs );
        nwrit = vsnprintf( stackbuf, 256, msg, stkargs );
        va_end( stkargs );
        if ( nwrit >= 256 )
        {
            heapbuf = priv_alloc( nwrit + 1 );
            if ( heapbuf )
            {
                (void)vsnprintf( heapbuf, nwrit + 1, msg, fmtargs );
                dispatch_error( file, code, heapbuf );
                priv_free( heapbuf );
            }
            else
                dispatch_error( file, code, "Unable to allocate temporary memory" );
        }
        else
            dispatch_error( file, code, stackbuf );
    }
    va_end( fmtargs );
    return code;
}

/**************************************/

static void priv_destroy_parts( EXR_TYPE(PRIV_FILE) *f )
{
    uint64_t *ctable;
    EXR_TYPE(PRIV_PART) nil = {0};
    for ( int p = 0; p < f->num_parts; ++p )
    {
        EXR_TYPE(PRIV_PART) *cur = f->parts[p];

        EXR_FUN(attr_list_destroy)( &(cur->attributes) );

        /* we stack x and y together so only have to free the first */
        priv_free( cur->tile_level_tile_count_x );

        ctable = (uint64_t *)atomic_load( &(cur->chunk_table) );
        priv_free( ctable );

        /* the first one is always the one that is part of the file */
        if ( p > 0 )
        {
            priv_free( cur );
        }
        else
        {
            *cur = nil;
        }
    }

    if ( f->num_parts > 1 )
        priv_free( f->parts );
    f->parts = NULL;
    f->num_parts = 0;
}

/**************************************/

int priv_add_part( EXR_TYPE(PRIV_FILE) *f, EXR_TYPE(PRIV_PART) **outpart )
{
    int ncount = f->num_parts + 1;
    EXR_TYPE(PRIV_PART) *part;
    EXR_TYPE(PRIV_PART) **nptrs = NULL;

    if ( ncount == 1 )
    {
        /* no need to zilch, the parent struct will have already been zero'ed */
        part = &(f->first_part);
        f->init_part = part;
        nptrs = &(f->init_part);
    }
    else
    {
        EXR_TYPE(PRIV_PART) nil = {0};

        part = priv_alloc( sizeof(EXR_TYPE(PRIV_PART)) );
        if ( ! part )
            return f->standard_error( (EXR_TYPE(FILE) *)f, EXR_DEF(ERR_OUT_OF_MEMORY) );

        nptrs = priv_alloc( sizeof(EXR_TYPE(PRIV_PART) *) * ncount );
        if ( ! nptrs )
        {
            priv_free( part );
            return f->standard_error( (EXR_TYPE(FILE) *)f, EXR_DEF(ERR_OUT_OF_MEMORY) );
        }
        *part = nil;
    }

    /* assign appropriately invalid values */
    part->storage_mode = EXR_DEF(STORAGE_LAST_TYPE);
    part->data_window.x_max = -1;
    part->data_window.y_max = -1;
    part->display_window.x_max = -1;
    part->display_window.y_max = -1;
    part->chunk_count = -1;

    /* put it into the part table */
    if ( ncount > 1 )
    {
        for ( int p = 0; p < f->num_parts; ++p )
        {
            nptrs[p] = f->parts[p];
        }
        nptrs[ncount - 1] = part;
    }

    if ( f->num_parts > 1 )
    {
        priv_free( f->parts );
    }
    f->parts = nptrs;
    f->num_parts = ncount;
    if ( outpart )
        *outpart = part;

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

int priv_create_file( EXR_TYPE(PRIV_FILE) **out, EXR_TYPE(error_handler_cb) errcb, size_t userdatasz, int isread )
{
    uint8_t *ptr;
    EXR_TYPE(PRIV_FILE) *ret, nil = {0};

    ptr = (uint8_t *)priv_alloc( sizeof(*ret) + userdatasz );
    if ( ptr == NULL )
    {
        if ( errcb )
            (*errcb)( NULL, EXR_DEF(ERR_OUT_OF_MEMORY),
                      EXR_FUN(get_default_error_message)( EXR_DEF(ERR_OUT_OF_MEMORY) ) );
        return EXR_DEF(ERR_OUT_OF_MEMORY);
    }
    ret = (EXR_TYPE(PRIV_FILE) *)ptr;
    *ret = nil;

    if ( userdatasz > 0 )
        ret->user_data = ptr + sizeof(*ret);
    ret->error_cb = errcb;

    ret->standard_error = &dispatch_standard_error;
    ret->report_error = &dispatch_error;
    ret->print_error = &dispatch_print_error;

    ret->file_size = -1;
    ret->max_name_length = EXR_SHORTNAME_MAXLEN;

    /* the first part will have been nil'ed out by our 0 struct,
     * but set properly invalid arguments */
    if ( isread )
    {
        EXR_TYPE(PRIV_PART) *part;
        /* we know it won't be allocating, so there won't be anything to fail */
        priv_add_part( ret, &part );
    }

    *out = ret;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

void priv_destroy_file( EXR_TYPE(PRIV_FILE) *pf )
{
    EXR_FUN(attr_string_destroy)( &(pf->filename) );
    EXR_FUN(attr_string_destroy)( &(pf->tmp_filename) );
    EXR_FUN(attr_list_destroy)( &(pf->custom_handlers) );
    priv_destroy_parts( pf );

    priv_free( pf );
}
