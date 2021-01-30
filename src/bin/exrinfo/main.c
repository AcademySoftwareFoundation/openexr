/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
# include <io.h>
# include <fcntl.h>
#else
# include <unistd.h>
#endif

#include <stdlib.h>

static void
usage( const char *argv0 )
{
	fprintf( stderr, "Usage: %s [-v|--verbose] <filename> [<filename> ...]\n\n", argv0 );
}

static void error_handler_cb( exr_file_t *f, int code, const char *msg )
{
    const char *fn = "<error>";
    if ( f )
        fn = exr_get_file_name( f );
	fprintf( stderr, "ERROR '%s' (%d): %s\n", fn, code, msg );
}

static int64_t stdin_reader(
    exr_file_t *file,
    void *userdata, void *buffer, uint64_t sz, uint64_t offset,
    exr_stream_error_func_ptr_t error_cb )
{
    static uint64_t lastoffset = 0;
    int64_t nread = 0;
    if ( offset != lastoffset )
    {
        error_cb( file, EXR_ERR_READ_IO, "Unable to seek in stdin stream" );
        return -1;
    }
#ifdef _WIN32
    if ( sz >= (size_t)(INT32_MAX) )
    {
        error_cb( file, EXR_ERR_READ_IO, "Read request too large for win32 API" );
        return -1;
    }
    nread = _read( _fileno(stdin), buffer, (unsigned)sz );
#else
    nread = read( STDIN_FILENO, buffer, sz );
#endif
    if ( nread > 0 )
        lastoffset = offset + nread;
    return nread;
}


static int process_stdin( int verbose )
{
    int rv;
    exr_file_t *e = NULL;

#ifdef _WIN32
    _setmode( _fileno(stdin), _O_BINARY );
#endif
    rv = exr_start_read_stream( &e, "<stdin>", NULL, stdin_reader, NULL, NULL, error_handler_cb );
	if ( rv == 0 )
	{
        exr_print_info( e, verbose );
		exr_close( &e );
    }
    return rv;
}

static int process_file( const char *filename, int verbose )
{
    int rv;
    exr_file_t *e = NULL;

    rv = exr_start_read( &e, filename, error_handler_cb );
	if ( rv == 0 )
	{
        exr_print_info( e, verbose );
		exr_close( &e );
    }
    return rv;
}

int
main( int argc, const char *argv[] )
{
	const char *filename = NULL;
	int rv = 0, nfiles = 0, verbose = 0;

    for ( int a = 1; a < argc; ++a )
    {
		if ( ! strcmp( argv[a], "-h" ) ||
			 ! strcmp( argv[a], "-?" ) ||
			 ! strcmp( argv[a], "--help" ) )
		{
			usage( argv[0] );
			return 0;
		}
		else if ( ! strcmp( argv[a], "-v" ) ||
                  ! strcmp( argv[a], "--verbose" ) )
        {
			verbose = 1;
        }
		else if ( ! strcmp( argv[a], "-" ) )
        {
            ++nfiles;
            rv += process_stdin( verbose );
        }
        else if ( argv[a][0] == '-' )
        {
            usage( argv[0] );
            return 1;
        }
        else
        {
            ++nfiles;
            rv += process_file( argv[a], verbose );
        }
	}

	return rv;
}
