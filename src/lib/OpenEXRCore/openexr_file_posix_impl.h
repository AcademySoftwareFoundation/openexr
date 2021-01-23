/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

/* implementation for unix-like file io routines (used in openexr_file.c) */

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
} exr_default_filehandle_t;
#else
typedef struct
{
    int fd;
    pthread_mutex_t mutex;
} exr_default_filehandle_t;
#endif

/**************************************/

static void default_shutdown( exr_file_t *f, void *userdata, int failed )
{
    /* we will handle failure before here */
    exr_default_filehandle_t *fh = userdata;
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

static int finalize_write( exr_PRIV_FILE_t *pf, int failed )
{
    int rv = EXR_ERR_SUCCESS;

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
                pf, EXR_ERR_FILE_ACCESS,
                "Unable to rename temporary file: %s", strerror(rv) );
    }

    return rv;
}

/**************************************/

static exr_ssize_t default_read_func(
    exr_file_t *file,
    void *userdata,
    void *buffer,
    size_t sz,
    off_t offset,
    exr_stream_error_func_ptr_t error_cb )
{
    exr_ssize_t rv, retsz = -1;
    exr_default_filehandle_t *fh = userdata;
    int fd = -1;
    char *curbuf = (char *)buffer;
    size_t readsz = sz;

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd < 0 )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file descriptor" );
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
                    error_cb( file, EXR_ERR_READ_IO, strerror( errno ) );
                else
                    error_cb( file, EXR_ERR_READ_IO, "Unable to seek to requested position" );
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
        error_cb( file, EXR_ERR_READ_IO,
                  "Unable to read %lu bytes: %s", sz, strerror( errno ) );
    return retsz;
}

/**************************************/

static exr_ssize_t default_write_func(
    exr_file_t *file,
    void *userdata,
    const void *buffer,
    size_t sz,
    off_t offset,
    exr_stream_error_func_ptr_t error_cb )
{
    exr_ssize_t rv, retsz = -1;
    exr_default_filehandle_t *fh = userdata;
    int fd = -1;
    const uint8_t *curbuf = (const uint8_t *)buffer;
    size_t readsz = sz;

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd < 0 )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file descriptor" );
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
                    error_cb( file, EXR_ERR_WRITE_IO, strerror( errno ) );
                else
                    error_cb( file, EXR_ERR_WRITE_IO, "Unable to seek to requested position" );
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
    if ( retsz != (exr_ssize_t)sz && error_cb )
        error_cb( file, EXR_ERR_WRITE_IO,
                  "Unable to write %lu bytes to stream, wrote %ld: %s",
                  sz, retsz, strerror( errno ) );
    return retsz;
}

/**************************************/

static int default_init_read_file( exr_PRIV_FILE_t *file )
{
    int rv, fd;
    exr_default_filehandle_t *fh = file->user_data;

    fh->fd = -1;
#if ! CAN_USE_PREAD
    rv = pthread_mutex_init( &(fh->mutex), NULL );
    if ( rv != 0 )
        return file->print_error( file, EXR_ERR_OUT_OF_MEMORY,
                                  "Unable to initialize file mutex: %s", strerror( rv ) );
#endif

    file->destroy_fn = &default_shutdown;
    file->read_fn = &default_read_func;

    fd = open( file->filename.str, O_RDONLY|O_CLOEXEC );
    if ( fd < 0 )
        return file->print_error(
            file, EXR_ERR_FILE_ACCESS,
            "Unable to open file for read: %s", strerror( errno ) );

    fh->fd = fd;
    return EXR_ERR_SUCCESS;
}

/**************************************/

static int default_init_write_file( exr_PRIV_FILE_t *file )
{
    int rv, fd;
    exr_default_filehandle_t *fh = file->user_data;
    const char *outfn = file->tmp_filename.str;
    if ( outfn == NULL )
        outfn = file->filename.str;

#if ! CAN_USE_PREAD
    rv = pthread_mutex_init( &(fh->mutex), NULL );
    if ( rv != 0 )
        return file->print_error( file, EXR_ERR_OUT_OF_MEMORY,
                                  "Unable to initialize file mutex: %s", strerror( rv ) );
#endif

    fh->fd = -1;
    file->destroy_fn = &default_shutdown;
    file->write_fn = &default_write_func;

    fd = open( outfn, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC,
               S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH );
    if ( fd < 0 )
        return file->print_error(
            file, EXR_ERR_FILE_ACCESS,
            "Unable to open file for write: %s", strerror( errno ) );
    fh->fd = fd;

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int default_query_size( exr_PRIV_FILE_t *file )
{
    int rv;
    struct stat sbuf;
    exr_default_filehandle_t *fh = file->user_data;

    file->file_size = -1;
    if ( fh->fd >= 0 )
    {
        rv = fstat( fh->fd, &sbuf );
        if ( rv == 0 )
            file->file_size = sbuf.st_size;
        else
            return file->print_error(
                file, EXR_ERR_FILE_ACCESS,
                "Unable to query file size: '%s'", strerror( errno ) );
    }
    else
        return file->standard_error( file, EXR_ERR_NOT_OPEN_READ );

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int make_temp_filename( exr_PRIV_FILE_t *ret )
{
    /* we checked the pointers we care about before calling */
    char tmproot[32];
    char *tmpname;
    size_t tlen, newlen;
    const char *srcfile = ret->filename.str;
    int nwr = snprintf( tmproot, 32, "tmp.%d", getpid() );
    if ( nwr >= 32 )
        return ret->report_error( ret, EXR_ERR_INVALID_ARGUMENT,
                                  "Invalid assumption in temporary filename" );

    tlen = strlen( tmproot );
    newlen = tlen + (size_t)ret->filename.length;

    if ( newlen >= INT32_MAX )
        return ret->standard_error( ret, EXR_ERR_OUT_OF_MEMORY );

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
        return ret->print_error( ret, EXR_ERR_OUT_OF_MEMORY,
                                 "Unable to create %lu bytes for temporary filename", (unsigned long)newlen + 1 );
    return EXR_ERR_SUCCESS;
}
