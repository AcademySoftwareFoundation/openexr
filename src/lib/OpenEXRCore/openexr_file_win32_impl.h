/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

/* implementation for unix-like file io routines (used in openexr_file.c) */

#include <windows.h>
#include <fileapi.h>
#include <strsafe.h>

static int print_error_helper( exr_PRIV_FILE_t *pf,
                               int errcode,
                               DWORD dw,
                               exr_stream_error_func_ptr_t error_cb,
                               const char *msg ) 
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    size_t bufsz = 0;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, 
        NULL );

    bufsz = ( lstrlen((LPCTSTR)lpMsgBuf)
              + strlen( msg )
              + 20 ); /* extra for format string */

    lpDisplayBuf = (LPVOID)LocalAlloc( LMEM_ZEROINIT, bufsz * sizeof(TCHAR) );
    
    if ( FAILED( StringCchPrintf((LPTSTR)lpDisplayBuf, bufsz,
                                 TEXT("%s: (%ld) %s"), 
                                 msg, 
                                 dw, 
                                 lpMsgBuf) ) )
    {
        return pf->print_error(
            pf, EXR_ERR_OUT_OF_MEMORY,
            "Unable to format message print" );
    }

    if ( error_cb )
        error_cb( pf, errcode, (const char *)lpDisplayBuf );
    else
        pf->print_error( pf, errcode, (const char *)lpDisplayBuf );

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

    return errcode;
}

static int print_error( exr_PRIV_FILE_t *pf, int errcode, const char *msg ) 
{
    return print_error_helper( pf, errcode, GetLastError(), NULL, msg );
}

static int send_error( exr_PRIV_FILE_t *pf, int errcode, exr_stream_error_func_ptr_t error_cb, const char *msg )
{
    return print_error_helper( pf, errcode, GetLastError(), error_cb, msg );
}

static wchar_t *widen_filename( const char *fn )
{
    int wcSize = 0, fnlen = 0;
    wchar_t *wcFn = NULL;

    fnlen = (int)strlen( fn );
    wcSize = MultiByteToWideChar( CP_UTF8, 0, fn, fnlen, NULL, 0 );
    wcFn = priv_alloc( sizeof(wchar_t) * ( wcSize + 1 ) );
    if ( wcFn )
    {
        MultiByteToWideChar( CP_UTF8, 0, fn, fnlen, wcFn, wcSize );
        wcFn[wcSize] = 0;
    }
    return wcFn;
}

typedef struct
{
    HANDLE fd;
} exr_default_filehandle_t;

/**************************************/

static void default_shutdown( exr_file_t *f, void *userdata, int failed )
{
    /* we will handle failure before here */
    exr_default_filehandle_t *fh = userdata;
    if ( fh )
    {
        if ( fh->fd != INVALID_HANDLE_VALUE )
            CloseHandle( fh->fd );
        fh->fd = INVALID_HANDLE_VALUE;
    }
}

/**************************************/

static int finalize_write( exr_PRIV_FILE_t *pf, int failed )
{
    /* TODO: Do we actually want to do this or leave the garbage file there */
    if ( failed && pf->destroy_fn == &default_shutdown )
    {
        wchar_t *wcFn;
        if (  pf->tmp_filename.str )
            wcFn = widen_filename( pf->tmp_filename.str );
        else
            wcFn = widen_filename( pf->filename.str );
        if ( wcFn )
        {
            DeleteFileW( wcFn );
            priv_free( wcFn );
        }
    }

    if ( !failed && pf->tmp_filename.str )
    {
        wchar_t *wcFnTmp = widen_filename( pf->tmp_filename.str );
        wchar_t *wcFn = widen_filename( pf->filename.str );
        BOOL res = FALSE;
        if ( wcFn && wcFnTmp )
            res = ReplaceFileW( wcFn, wcFnTmp, NULL, 0, NULL, NULL );
        priv_free( wcFn );
        priv_free( wcFnTmp );

        if ( ! res )
            return print_error( pf, EXR_ERR_FILE_ACCESS,
                                "Unable to rename temporary file to final destination" );
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

static exr_ssize_t default_read_func(
    exr_file_t *file,
    void *userdata,
    void *buffer,
    size_t sz,
    exr_off_t offset,
    exr_stream_error_func_ptr_t error_cb )
{
    exr_ssize_t retsz = -1;
    DWORD nread = 0;
    exr_default_filehandle_t *fh = userdata;
    HANDLE fd;
    LARGE_INTEGER lint;
    OVERLAPPED overlap = { 0 };

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd == INVALID_HANDLE_VALUE )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file descriptor" );
        return retsz;
    }

    if ( sz > (size_t)(INT32_MAX) )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Read request too large for win32 api" );
        return retsz;
    }

    lint.QuadPart = offset;
    overlap.Offset = lint.LowPart;
    overlap.OffsetHigh = lint.HighPart;
    if ( ReadFile( fd, buffer, (DWORD)sz, &nread, &overlap ) )
    {
        retsz = nread;
    }
    else
    {
        DWORD dw = GetLastError();
        if ( dw != ERROR_HANDLE_EOF )
        {
            print_error_helper( EXR_GETFILE(file), EXR_ERR_READ_IO, dw, error_cb, "Unable to read requested data" );
        }
        else
        {
            retsz = nread;
        }
    }

    return retsz;
}

/**************************************/

static exr_ssize_t default_write_func(
    exr_file_t *file,
    void *userdata,
    const void *buffer,
    size_t sz,
    exr_off_t offset,
    exr_stream_error_func_ptr_t error_cb )
{
    exr_ssize_t retsz = -1;
    exr_default_filehandle_t *fh = userdata;
    HANDLE fd;
    const uint8_t *curbuf = buffer;

    if ( ! fh )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file handle pointer" );
        return retsz;
    }
    
    fd = fh->fd;
    if ( fd == INVALID_HANDLE_VALUE )
    {
        if ( error_cb )
            error_cb( file, EXR_ERR_INVALID_ARGUMENT, "Invalid file descriptor" );
        return retsz;
    }

    /****** TODO ******/
    if ( error_cb )
        error_cb( file, EXR_ERR_WRITE_IO,
                  "Win32 write function is NYI" );
    return retsz;
}

/**************************************/

static int default_init_read_file( exr_PRIV_FILE_t *file )
{
    wchar_t *wcFn = NULL;
    HANDLE fd;
    exr_default_filehandle_t *fh = file->user_data;

    fh->fd = INVALID_HANDLE_VALUE;
    file->destroy_fn = &default_shutdown;
    file->read_fn = &default_read_func;

    wcFn = widen_filename( file->filename.str );
    if ( wcFn )
    {
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        fd = CreateFile2( wcFn,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          OPEN_EXISTING,
                          NULL );
#else
        fd = CreateFileW( wcFn,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, /* TBD: use overlapped? | FILE_FLAG_OVERLAPPED */
                          NULL );
#endif
        priv_free( wcFn );

        if ( fd == INVALID_HANDLE_VALUE )
            return print_error( file, EXR_ERR_FILE_ACCESS, "Unable to open file for read" );
    }
    else
        return print_error( file, EXR_ERR_OUT_OF_MEMORY, "Unable to allocate unicode filename" );
        
    fh->fd = fd;

    return EXR_ERR_SUCCESS;
}

/**************************************/

static int default_init_write_file( exr_PRIV_FILE_t *file )
{
    wchar_t *wcFn = NULL;
    exr_default_filehandle_t *fh = file->user_data;
    HANDLE fd;
    const char *outfn = file->tmp_filename.str;

    if ( outfn == NULL )
        outfn = file->filename.str;

    fh->fd = INVALID_HANDLE_VALUE;
    file->destroy_fn = &default_shutdown;
    file->write_fn = &default_write_func;

    wcFn = widen_filename( outfn );
    if ( wcFn )
    {
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
        fd = CreateFile2( wcFn,
                          GENERIC_WRITE | DELETE,
                          0, /* no sharing */
                          CREATE_ALWAYS,
                          NULL );
#else
        fd = CreateFileW( wcFn,
                          GENERIC_WRITE | DELETE,
                          0, /* no sharing */
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, /* TBD: use overlapped? | FILE_FLAG_OVERLAPPED */
                          NULL );
#endif
        priv_free( wcFn );

        if ( fd == INVALID_HANDLE_VALUE )
            return print_error( file, EXR_ERR_FILE_ACCESS, "Unable to open file for write" );
    }
    else
        return print_error( file, EXR_ERR_OUT_OF_MEMORY, "Unable to allocate unicode filename" );

    fh->fd = fd;
    return EXR_ERR_SUCCESS;
}

/**************************************/

static int default_query_size( exr_PRIV_FILE_t *file )
{
    exr_default_filehandle_t *fh = file->user_data;

    file->file_size = -1;
    if ( fh->fd != INVALID_HANDLE_VALUE )
    {
        LARGE_INTEGER lint = { 0 };
        if ( GetFileSizeEx( fh->fd, &lint ) )
        {
            file->file_size = lint.QuadPart;
            return EXR_ERR_SUCCESS;
        }
        
        return print_error( file, EXR_ERR_FILE_ACCESS,
                            "Unable to query file size" );
    }

    return file->standard_error( file, EXR_ERR_NOT_OPEN_READ );
}

/**************************************/

static int make_temp_filename( exr_PRIV_FILE_t *ret )
{
    /* we checked the pointers we care about before calling */
    char tmproot[32];
    char *tmpname;
    size_t tlen, newlen;
    const char *srcfile = ret->filename.str;
    int nwr = _snprintf_s( tmproot, 32, _TRUNCATE, "tmp.%d", GetCurrentProcessId() );
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
        // windows allows both
        const char *lastslash = strrchr( srcfile, '\\' );
        const char *lastslashu = strrchr( srcfile, '/' );

        if ( lastslash )
        {
            if ( lastslashu && lastslashu > lastslash )
                lastslash = lastslashu;
        }
        else
            lastslash = lastslashu;

        ret->tmp_filename.length = (int32_t)(newlen);
        ret->tmp_filename.alloc_size = (int32_t)(newlen + 1);
        ret->tmp_filename.str = tmpname;

        if ( lastslash )
        {
            size_t nPrev = (uintptr_t)lastslash - (uintptr_t)srcfile + 1;
            strncpy_s( tmpname, newlen + 1, srcfile, nPrev );
            strncpy_s( tmpname + nPrev, newlen + 1 - nPrev, tmproot, tlen );
            strncpy_s( tmpname + nPrev + tlen, newlen + 1 - nPrev - tlen,
                       srcfile + nPrev, ret->filename.length - nPrev );
            tmpname[newlen] = '\0';
        }
        else
        {
            strncpy_s( tmpname, newlen + 1, tmproot, tlen );
            strncpy_s( tmpname + tlen, newlen + 1 - tlen, srcfile, ret->filename.length );
            tmpname[newlen] = '\0';
        }
    }
    else
        return ret->print_error( ret, EXR_ERR_OUT_OF_MEMORY,
                                 "Unable to create %lu bytes for temporary filename", (unsigned long)newlen + 1 );
    return EXR_ERR_SUCCESS;
}
