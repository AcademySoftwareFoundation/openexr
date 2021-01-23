/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr.h"

#include "openexr_priv_file.h"
#include "openexr_priv_constants.h"
#include "openexr_priv_memory.h"

#if defined(_WIN32) || defined(_WIN64)
# include "openexr_file_win32_impl.h"
#else
# include "openexr_file_posix_impl.h"
#endif

/**************************************/

static int dispatch_read( exr_file_t *f, void *buf, size_t sz, off_t *offsetp, exr_ssize_t *nread, __PRIV_READ_MODE rmode )
{
    exr_PRIV_FILE_t *file = EXR_GETFILE(f);
    exr_ssize_t rval = -1;
    int rv = EXR_ERR_UNKNOWN;

    if ( nread )
        *nread = rval;

    if ( ! file )
        return file->report_error( f, EXR_ERR_INVALID_ARGUMENT, "No file provided for read" );

    if ( ! offsetp )
        return file->report_error(
            file, EXR_ERR_INVALID_ARGUMENT,
            "read requested with no output offset pointer" );

    if ( file->read_fn )
        rval = file->read_fn( file, file->user_data, buf, sz, *offsetp, file->print_error );
    else
        return file->standard_error( f, EXR_ERR_NOT_OPEN_READ );

    if ( nread )
        *nread = rval;
    if ( rval > 0 )
        *offsetp += rval;

    if ( rval == (exr_ssize_t)sz || ( rmode == EXR_ALLOW_SHORT_READ && rval >= 0 ) )
        rv = EXR_ERR_SUCCESS;
    else
        rv = EXR_ERR_READ_IO;
    return rv;
}

/**************************************/

static int dispatch_write( exr_file_t *f, const void *buf, size_t sz, off_t *offsetp )
{
    exr_PRIV_FILE_t *file = EXR_GETFILE(f);
    exr_ssize_t rval = -1;
    int rv = EXR_ERR_UNKNOWN;
    off_t outpos;

    if ( ! file )
        return file->report_error( f, EXR_ERR_INVALID_ARGUMENT, "No file provided for write" );

    if ( ! offsetp )
        return file->report_error(
            file, EXR_ERR_INVALID_ARGUMENT,
            "write requested with no output offset pointer" );

    outpos = atomic_fetch_add( &(file->file_offset), (off_t)sz );
    if ( file->write_fn )
        rval = file->write_fn( file, file->user_data, buf, sz, outpos, file->print_error );
    else
        return file->standard_error( f, EXR_ERR_NOT_OPEN_WRITE );

    if ( rval > 0 )
        *offsetp = outpos + rval;
    else
        *offsetp = outpos;
    
    if ( rval == (exr_ssize_t)sz )
        rv = EXR_ERR_SUCCESS;
    else
        rv = EXR_ERR_READ_IO;
    return rv;
}

/**************************************/

int exr_start_read(
    exr_file_t **file,
    const char *filename,
    exr_error_handler_cb_t error_cb )
{
    int rv = EXR_ERR_UNKNOWN;
    exr_PRIV_FILE_t *ret = NULL;

    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( filename && filename[0] != '\0' )
    {
        rv = priv_create_file( &ret, error_cb, sizeof(exr_default_filehandle_t), 1 );
        if ( rv == EXR_ERR_SUCCESS )
        {
            ret->do_read = &dispatch_read;
            ret->do_write = &dispatch_write;

            rv = exr_attr_string_create( ret, &(ret->filename), filename );
            if ( rv == EXR_ERR_SUCCESS )
            {
                rv = default_init_read_file( ret );

                if ( rv == EXR_ERR_SUCCESS )
                    rv = default_query_size( ret );
                if ( rv == EXR_ERR_SUCCESS )
                    rv = priv_parse_header( ret );
            }
            if ( rv != EXR_ERR_SUCCESS )
                exr_close( (exr_file_t **)&ret );
        }
    }
    else
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid filename passed to start_read function" );
        else
            fprintf( stderr, "Invalid filename passed to start_read function\n" );
        rv = EXR_ERR_INVALID_ARGUMENT;
    }

    *file = (exr_file_t *)ret;
    return rv;
}

/**************************************/

int exr_start_read_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_read_func_ptr_t read_fn,
    exr_query_size_func_ptr_t size_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb )
{
    int rv = EXR_ERR_UNKNOWN;
    exr_PRIV_FILE_t *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( ! read_fn )
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Missing stream read function to start_read function" );
        else
            fprintf( stderr, "Missing stream read function to start_read function\n" );
        return EXR_ERR_INVALID_ARGUMENT;
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
            rv = exr_attr_string_create( ret, &(ret->filename), streamname );

        if ( size_fn )
            ret->file_size = size_fn( ret, userdata );
        else
            ret->file_size = -1;

        if ( rv == 0 )
            rv = priv_parse_header( ret );

        if ( rv != 0 )
            exr_close( (exr_file_t **)&ret );
    }
    else
    {
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
    }
    
    *file = (exr_file_t *)ret;
    return rv;
}

/**************************************/

int exr_start_write(
    exr_file_t **file,
    const char *filename,
    int use_tempfile,
    exr_error_handler_cb_t error_cb )
{
    int rv = EXR_ERR_UNKNOWN;
    exr_PRIV_FILE_t *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        return EXR_ERR_INVALID_ARGUMENT;
    }

    if ( filename && filename[0] != '\0' )
    {
        rv = priv_create_file( &ret, error_cb, sizeof(exr_default_filehandle_t), 0 );
        if ( rv == 0 )
        {
            ret->do_read = &dispatch_read;
            ret->do_write = &dispatch_write;

            rv = exr_attr_string_create( ret, &(ret->filename), filename );
            if ( rv == 0 && use_tempfile )
                rv = make_temp_filename( ret );

            if ( rv == 0 )
                rv = default_init_write_file( ret );

            if ( rv != 0 )
                exr_close( (exr_file_t **)&ret );
        }
    }
    else if ( error_cb )
        error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid filename passed to start_write function" );
    else
        fprintf( stderr, "Invalid filename passed to start_write function\n" );

    *file = (exr_file_t *)ret;
    return rv;
}

/**************************************/

int exr_start_write_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_write_func_ptr_t write_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb )
{
    int rv = EXR_ERR_UNKNOWN;
    exr_PRIV_FILE_t *ret = NULL;
    if ( ! file )
    {
        if ( error_cb )
            error_cb( NULL, EXR_ERR_INVALID_ARGUMENT, "Invalid file output handle passed to start_read function" );
        else
            fprintf( stderr, "Invalid output file handle pointer passed to start_read function\n" );
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
        return EXR_ERR_INVALID_ARGUMENT;
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
            rv = exr_attr_string_create( ret, &(ret->filename), streamname );

        if ( rv != 0 )
            exr_close( (exr_file_t **)&ret );
    }
    else
    {
        if ( destroy_fn )
            destroy_fn( NULL, userdata, 1 );
    }

    *file = (exr_file_t *)ret;
    return rv;
}

/**************************************/

int exr_start_inplace_header_update(
    exr_file_t **file,
    const char *filename,
    exr_error_handler_cb_t error_cb )
{
    return EXR_ERR_INVALID_ARGUMENT;
}

/**************************************/

int exr_start_inplace_header_update_stream(
    exr_file_t **file,
    const char *streamname,
    void *userdata,
    exr_read_func_ptr_t read_fn,
    exr_query_size_func_ptr_t size_fn,
    exr_write_func_ptr_t write_fn,
    exr_destroy_stream_func_ptr_t destroy_fn,
    exr_error_handler_cb_t error_cb )
{
    return EXR_ERR_INVALID_ARGUMENT;
}

/**************************************/

int exr_close( exr_file_t **f )
{
    int rv = 0;
    if ( ! f )
        return EXR_ERR_INVALID_ARGUMENT;

    exr_PRIV_FILE_t *pf = EXR_GETFILE(*f);
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

