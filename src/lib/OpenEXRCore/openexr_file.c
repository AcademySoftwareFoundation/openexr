/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr.h"

#include "openexr_priv_file.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#ifdef _WIN32
# define USE_WIN32_FILEIO
#endif

#ifdef USE_WIN32_FILEIO
# error "Lazy, TODO"
#else

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#if defined __USE_UNIX98 || defined __USE_XOPEN2K8
# define CAN_USE_PREAD 1
#else
# define CAN_USE_PREAD 0
#endif

#if CAN_USE_PREAD
typedef struct
{
    int fd;
} EXR_TYPE(default_filehandle);
#else
typedef struct
{
    int fd;
    pthread_mutex_t mutex;
} EXR_TYPE(default_filehandle);
#endif

/**************************************/

static void default_shutdown( EXR_TYPE(FILE) *f, void *userdata, int failed )
{
    /* we will handle failure before here */
    EXR_TYPE(default_filehandle) *fh = userdata;
    if ( fh )
    {
        if ( fh->fd >= 0 )
            close( fh->fd );
#if ! CAN_USE_PREAD
        pthread_mutex_destroy( &(fh->mutex) );
#endif
    }
}

/**************************************/

static int finalize_write( EXR_TYPE(PRIV_FILE) *pf, int failed )
{
    int rv = EXR_DEF(ERR_SUCCESS);

    /* TODO: Do we actually want to do this or leave the garbage file there */
    if ( failed && pf->destroy_fn == &default_shutdown )
    {
        if (  pf->tmp_filename.str )
            unlink( pf->tmp_filename.str );
        else
            unlink( pf->filename.str );
    }

    if ( !failed && pf->tmp_filename.str )
    {
        int mvret = rename( pf->tmp_filename.str, pf->filename.str );
        if ( mvret < 0 )
            return pf->print_error(
                pf, EXR_DEF(ERR_FILE_ACCESS),
                "Unable to rename temporary file: %s", strerror(rv) );
    }

    return rv;
}

/**************************************/

static ssize_t default_read_func(
    EXR_TYPE(FILE) *file,
    void *userdata,
    void *buffer,
    size_t sz,
    off_t offset,
    EXR_TYPE(stream_error_func_ptr) error_cb )
{
    ssize_t rv, retsz = -1;
    EXR_TYPE(default_filehandle) *fh = userdata;
    int fd = -1;
    char *curbuf = (char *)buffer;
    size_t readsz = sz;

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd < 0 )
    {
        if ( error_cb )
            error_cb( file, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file descriptor" );
        return retsz;
    }

#if ! CAN_USE_PREAD
    pthread_mutex_lock( &(fh->mutex) );
    {
        off_t spos = lseek( fd, offset, SEEK_SET );
        if ( spos != offset )
        {
            pthread_mutex_unlock( &(fh->mutex) );
            if ( error_cb )
            {
                if ( spos == (off_t)-1 )
                    error_cb( file, EXR_DEF(ERR_READ_IO), strerror( errno ) );
                else
                    error_cb( file, EXR_DEF(ERR_READ_IO), "Unable to seek to requested position" );
            }
            return retsz;
        }
    }
#endif

    retsz = 0;
    do
    {
#if CAN_USE_PREAD
        rv = pread( fd, curbuf, readsz, offset );
#else
        rv = read( fd, curbuf, readsz );
#endif
        if ( rv < 0 )
        {
            if ( errno == EINTR )
                continue;
            if ( errno == EAGAIN )
                continue;
            retsz = -1;
            break;
        }
        if ( rv == 0 )
            break;
        retsz += rv;
        curbuf += rv;
        readsz -= rv;
        offset += rv;
    } while ( retsz < sz );

#if ! CAN_USE_PREAD
    pthread_mutex_unlock( &(fh->mutex) );
#endif
    if ( retsz < 0 && error_cb )
        error_cb( file, EXR_DEF(ERR_READ_IO),
                  "Unable to read %lu bytes: %s", sz, strerror( errno ) );
    return retsz;
}

/**************************************/

static ssize_t default_write_func(
    EXR_TYPE(FILE) *file,
    void *userdata,
    const void *buffer,
    size_t sz,
    off_t offset,
    EXR_TYPE(stream_error_func_ptr) error_cb )
{
    ssize_t rv, retsz = -1;
    EXR_TYPE(default_filehandle) *fh = userdata;
    int fd = -1;
    const uint8_t *curbuf = (const uint8_t *)buffer;
    size_t readsz = sz;

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd < 0 )
    {
        if ( error_cb )
            error_cb( file, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file descriptor" );
        return retsz;
    }

#if ! CAN_USE_PREAD
    pthread_mutex_lock( &(fh->mutex) );
    {
        off_t spos = lseek( fd, offset, SEEK_SET );
        if ( spos != offset )
        {
            pthread_mutex_unlock( &(fh->mutex) );
            if ( error_cb )
            {
                if ( spos == (off_t)-1 )
                    error_cb( file, EXR_DEF(ERR_WRITE_IO), strerror( errno ) );
                else
                    error_cb( file, EXR_DEF(ERR_WRITE_IO), "Unable to seek to requested position" );
            }
            return retsz;
        }
    }
#endif

    retsz = 0;
    do
    {
#if CAN_USE_PREAD
        rv = pwrite( fd, curbuf, readsz, offset );
#else
        rv = write( fd, curbuf, readsz );
#endif
        if ( rv < 0 )
        {
            if ( errno == EINTR )
                continue;
            if ( errno == EAGAIN )
                continue;
            retsz = -1;
            break;
        }
        retsz += rv;
        curbuf += rv;
        readsz -= rv;
        offset += rv;
    } while ( retsz < sz );

#if ! CAN_USE_PREAD
    pthread_mutex_unlock( &(fh->mutex) );
#endif
    if ( retsz != (ssize_t)sz && error_cb )
        error_cb( file, EXR_DEF(ERR_WRITE_IO),
                  "Unable to write %lu bytes to stream, wrote %ld: %s",
                  sz, retsz, strerror( errno ) );
    return retsz;
}

/**************************************/

static int default_init_read_file( EXR_TYPE(PRIV_FILE) *file )
{
    int rv, fd;
    EXR_TYPE(default_filehandle) *fh = file->user_data;

    fh->fd = -1;
#if ! CAN_USE_PREAD
    rv = pthread_mutex_init( &(fh->mutex), NULL );
    if ( rv != 0 )
        return file->print_error( file, EXR_DEF(ERR_OUT_OF_MEMORY),
                                  "Unable to initialize file mutex: %s", strerror( rv ) );
#endif

    file->destroy_fn = &default_shutdown;
    file->read_fn = &default_read_func;

    fd = open( file->filename.str, O_RDONLY|O_CLOEXEC );
    if ( fd < 0 )
        return file->print_error(
            file, EXR_DEF(ERR_FILE_ACCESS),
            "Unable to open file for read: %s", strerror( errno ) );

    fh->fd = fd;
    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int default_init_write_file( EXR_TYPE(PRIV_FILE) *file )
{
    int rv, fd;
    EXR_TYPE(default_filehandle) *fh = file->user_data;
    const char *outfn = file->tmp_filename.str;
    if ( outfn == NULL )
        outfn = file->filename.str;

#if ! CAN_USE_PREAD
    rv = pthread_mutex_init( &(fh->mutex), NULL );
    if ( rv != 0 )
        return file->print_error( file, EXR_DEF(ERR_OUT_OF_MEMORY),
                                  "Unable to initialize file mutex: %s", strerror( rv ) );
#endif

    fh->fd = -1;
    file->destroy_fn = &default_shutdown;
    file->write_fn = &default_write_func;

    fd = open( outfn, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,
               S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH );
    if ( fd < 0 )
        return file->print_error(
            file, EXR_DEF(ERR_FILE_ACCESS),
            "Unable to open file for write: %s", strerror( errno ) );
    fh->fd = fd;

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int default_query_size( EXR_TYPE(PRIV_FILE) *file )
{
    int rv;
    struct stat sbuf;
    EXR_TYPE(default_filehandle) *fh = file->user_data;

    file->file_size = -1;
    if ( fh->fd >= 0 )
    {
        rv = fstat( fh->fd, &sbuf );
        if ( rv == 0 )
            file->file_size = sbuf.st_size;
        else
            return file->print_error(
                file, EXR_DEF(ERR_FILE_ACCESS),
                "Unable to query file size: '%s'", strerror( errno ) );
    }
    else
        return file->standard_error( file, EXR_DEF(ERR_NOT_OPEN_READ) );

    return EXR_DEF(ERR_SUCCESS);
}

/**************************************/

static int make_temp_filename( EXR_TYPE(PRIV_FILE) *ret )
{
    /* we checked the pointers we care about before calling */
    char tmproot[32];
    char *tmpname;
    size_t tlen, newlen;
    const char *srcfile = ret->filename.str;
    int nwr = snprintf( tmproot, 32, "tmp.%d", getpid() );
    if ( nwr >= 32 )
        return ret->report_error( ret, EXR_DEF(ERR_INVALID_ARGUMENT),
                                  "Invalid assumption in temporary filename" );

    tlen = strlen( tmproot );
    newlen = tlen + (size_t)ret->filename.length;

    if ( newlen >= INT32_MAX )
        return ret->standard_error( ret, EXR_DEF(ERR_OUT_OF_MEMORY) );

    tmpname = priv_alloc( newlen + 1 );
    if ( tmpname )
    {
        const char *lastslash = strrchr( srcfile, '/' );

        ret->tmp_filename.length = (int32_t)(newlen);
        ret->tmp_filename.alloc_size = (int32_t)(newlen + 1);
        ret->tmp_filename.str = tmpname;

        if ( lastslash )
        {
            size_t nPrev = (uintptr_t)lastslash - (uintptr_t)srcfile + 1;
            strncpy( tmpname, srcfile, nPrev );
            strncpy( tmpname + nPrev, tmproot, tlen );
            strncpy( tmpname + nPrev + tlen, srcfile + nPrev, ret->filename.length - nPrev );
            tmpname[newlen] = '\0';
        }
        else
        {
            strncpy( tmpname, tmproot, tlen );
            strncpy( tmpname + tlen, srcfile, ret->filename.length );
            tmpname[newlen] = '\0';
        }
    }
    else
        return ret->print_error( ret, EXR_DEF(ERR_OUT_OF_MEMORY),
                                 "Unable to create %lu bytes for temporary filename", (unsigned long)newlen + 1 );
    return EXR_DEF(ERR_SUCCESS);
}

#endif /* unix-ish */

/**************************************/

static int dispatch_read( EXR_TYPE(FILE) *f, void *buf, size_t sz, off_t *offsetp, ssize_t *nread, __PRIV_READ_MODE rmode )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
    ssize_t rval = -1;
    int rv = EXR_DEF(ERR_UNKNOWN);

    if ( nread )
        *nread = rval;

    if ( ! file )
        return file->report_error( f, EXR_DEF(ERR_INVALID_ARGUMENT), "No file provided for read" );

    if ( ! offsetp )
        return file->report_error(
            file, EXR_DEF(ERR_INVALID_ARGUMENT),
            "read requested with no output offset pointer" );

    if ( file->read_fn )
        rval = file->read_fn( file, file->user_data, buf, sz, *offsetp, file->print_error );
    else
        return file->standard_error( f, EXR_DEF(ERR_NOT_OPEN_READ) );

    if ( nread )
        *nread = rval;
    if ( rval > 0 )
        *offsetp += rval;

    if ( rval == (ssize_t)sz || ( rmode == EXR_ALLOW_SHORT_READ && rval >= 0 ) )
        rv = EXR_DEF(ERR_SUCCESS);
    else
        rv = EXR_DEF(ERR_READ_IO);
    return rv;
}

/**************************************/

static int dispatch_write( EXR_TYPE(FILE) *f, const void *buf, size_t sz, off_t *offsetp )
{
    EXR_TYPE(PRIV_FILE) *file = EXR_GETFILE(f);
    ssize_t rval = -1;
    int rv = EXR_DEF(ERR_UNKNOWN);
    off_t outpos;

    if ( ! file )
        return file->report_error( f, EXR_DEF(ERR_INVALID_ARGUMENT), "No file provided for write" );

    if ( ! offsetp )
        return file->report_error(
            file, EXR_DEF(ERR_INVALID_ARGUMENT),
            "write requested with no output offset pointer" );

    outpos = atomic_fetch_add( &(file->file_offset), (off_t)sz );
    if ( file->write_fn )
        rval = file->write_fn( file, file->user_data, buf, sz, outpos, file->print_error );
    else
        return file->standard_error( f, EXR_DEF(ERR_NOT_OPEN_WRITE) );

    if ( rval > 0 )
        *offsetp = outpos + rval;
    else
        *offsetp = outpos;
    
    if ( rval == (ssize_t)sz )
        rv = EXR_DEF(ERR_SUCCESS);
    else
        rv = EXR_DEF(ERR_READ_IO);
    return rv;
}

/**************************************/

int EXR_FUN(start_read)(
    EXR_TYPE(FILE) **file,
    const char *filename,
    EXR_TYPE(error_handler_cb) error_cb )
{
    int rv = EXR_DEF(ERR_UNKNOWN);
    EXR_TYPE(PRIV_FILE) *ret = NULL;

    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( filename && filename[0] != '\0' )
    {
        rv = priv_create_file( &ret, error_cb, sizeof(EXR_TYPE(default_filehandle)), 1 );
        if ( rv == EXR_DEF(ERR_SUCCESS) )
        {
            ret->do_read = &dispatch_read;
            ret->do_write = &dispatch_write;

            rv = EXR_FUN(attr_string_create)( ret, &(ret->filename), filename );
            if ( rv == EXR_DEF(ERR_SUCCESS) )
            {
                rv = default_init_read_file( ret );

                if ( rv == EXR_DEF(ERR_SUCCESS) )
                    rv = default_query_size( ret );
                if ( rv == EXR_DEF(ERR_SUCCESS) )
                    rv = priv_parse_header( ret );
            }
            if ( rv != EXR_DEF(ERR_SUCCESS) )
                EXR_FUN(close)( (EXR_TYPE(FILE) **)&ret );
        }
    }
    else
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid filename passed to start_read function" );
        else
            fprintf( stderr, "Invalid filename passed to start_read function\n" );
        rv = EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    *file = (EXR_TYPE(FILE) *)ret;
    return rv;
}

/**************************************/

int EXR_FUN(start_read_stream)(
    EXR_TYPE(FILE) **file,
    const char *streamname,
    void *userdata,
    EXR_TYPE(read_func_ptr) read_fn,
    EXR_TYPE(query_size_func_ptr) size_fn,
    EXR_TYPE(destroy_stream_func_ptr) destroy_fn,
    EXR_TYPE(error_handler_cb) error_cb )
{
    int rv = EXR_DEF(ERR_UNKNOWN);
    EXR_TYPE(PRIV_FILE) *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( ! read_fn )
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Missing stream read function to start_read function" );
        else
            fprintf( stderr, "Missing stream read function to start_read function\n" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    rv = priv_create_file( &ret, error_cb, 0, 1 );
    if ( rv == 0 )
    {
        ret->do_read = &dispatch_read;
        ret->do_write = &dispatch_write;

        ret->user_data = userdata;
        ret->destroy_fn = destroy_fn;
        ret->read_fn = read_fn;

        if ( streamname )
            rv = EXR_FUN(attr_string_create)( ret, &(ret->filename), streamname );

        if ( size_fn )
            ret->file_size = size_fn( ret, userdata );
        else
            ret->file_size = -1;

        if ( rv == 0 )
            rv = priv_parse_header( ret );

        if ( rv != 0 )
            EXR_FUN(close)( (EXR_TYPE(FILE) **)&ret );
    }
    else
    {
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
    }
    
    *file = (EXR_TYPE(FILE) *)ret;
    return rv;
}

/**************************************/

int EXR_FUN(start_write)(
    EXR_TYPE(FILE) **file,
    const char *filename,
    int use_tempfile,
    EXR_TYPE(error_handler_cb) error_cb )
{
    int rv = EXR_DEF(ERR_UNKNOWN);
    EXR_TYPE(PRIV_FILE) *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    if ( filename && filename[0] != '\0' )
    {
        rv = priv_create_file( &ret, error_cb, sizeof(EXR_TYPE(default_filehandle)), 0 );
        if ( rv == 0 )
        {
            ret->do_read = &dispatch_read;
            ret->do_write = &dispatch_write;

            rv = EXR_FUN(attr_string_create)( ret, &(ret->filename), filename );
            if ( rv == 0 && use_tempfile )
                rv = make_temp_filename( ret );

            if ( rv == 0 )
                rv = default_init_write_file( ret );

            if ( rv != 0 )
                EXR_FUN(close)( (EXR_TYPE(FILE) **)&ret );
        }
    }
    else if ( error_cb )
        error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid filename passed to start_write function" );
    else
        fprintf( stderr, "Invalid filename passed to start_write function\n" );

    *file = (EXR_TYPE(FILE) *)ret;
    return rv;
}

/**************************************/

int EXR_FUN(start_write_stream)(
    EXR_TYPE(FILE) **file,
    const char *streamname,
    void *userdata,
    EXR_TYPE(write_func_ptr) write_fn,
    EXR_TYPE(destroy_stream_func_ptr) destroy_fn,
    EXR_TYPE(error_handler_cb) error_cb )
{
    int rv = EXR_DEF(ERR_UNKNOWN);
    EXR_TYPE(PRIV_FILE) *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_DEF(ERR_INVALID_ARGUMENT), "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
        return EXR_DEF(ERR_INVALID_ARGUMENT);
    }

    rv = priv_create_file( &ret, error_cb, 0, 0 );
    if ( rv == 0 )
    {
        ret->do_read = &dispatch_read;
        ret->do_write = &dispatch_write;

        ret->user_data = userdata;
        ret->destroy_fn = destroy_fn;
        ret->write_fn = write_fn;

        if ( streamname )
            rv = EXR_FUN(attr_string_create)( ret, &(ret->filename), streamname );

        if ( rv != 0 )
            EXR_FUN(close)( (EXR_TYPE(FILE) **)&ret );
    }
    else
    {
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
    }

    *file = (EXR_TYPE(FILE) *)ret;
    return rv;
}

/**************************************/

int EXR_FUN(start_inplace_header_update)(
    EXR_TYPE(FILE) **file,
    const char *filename,
    EXR_TYPE(error_handler_cb) error_cb )
{
    return EXR_DEF(ERR_INVALID_ARGUMENT);
}

/**************************************/

int EXR_FUN(start_inplace_header_update_stream)(
    EXR_TYPE(FILE) **file,
    const char *streamname,
    void *userdata,
    EXR_TYPE(read_func_ptr) read_fn,
    EXR_TYPE(query_size_func_ptr) size_fn,
    EXR_TYPE(write_func_ptr) write_fn,
    EXR_TYPE(destroy_stream_func_ptr) destroy_fn,
    EXR_TYPE(error_handler_cb) error_cb )
{
    return EXR_DEF(ERR_INVALID_ARGUMENT);
}

/**************************************/

int EXR_FUN(close)( EXR_TYPE(FILE) **f )
{
    int rv = 0;
    if ( ! f )
        return EXR_DEF(ERR_INVALID_ARGUMENT);

    EXR_TYPE(PRIV_FILE) *pf = EXR_GETFILE(*f);
    *f = NULL;

    if ( pf )
    {
        int failed = 0;
        if ( pf->write_fn )
        {
            printf( "TODO: check all chunks written and write chunk offsets\n" );
            failed = 1;

            rv = finalize_write( pf, failed );
        }

        if ( pf->destroy_fn )
            pf->destroy_fn( *f, pf->user_data, failed );

        priv_destroy_file( pf );
    }

    return rv;
}

