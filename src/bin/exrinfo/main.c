/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include <openexr.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static void
usage( const char *argv0 )
{
	fprintf( stderr, "Usage: %s [-v|--verbose] <filename> [<filename> ...]\n\n", argv0 );
}

static void error_handler_cb( EXR_TYPE(FILE) *f, int code, const char *msg )
{
    const char *fn = "<error>";
    if ( f )
        fn = EXR_FUN(get_file_name)( f );
	fprintf( stderr, "ERROR '%s' (%d): %s\n", fn, code, msg );
}

static ssize_t stdin_reader(
    EXR_TYPE(FILE) *file,
    void *userdata, void *buffer, size_t sz, off_t offset,
    EXR_TYPE(stream_error_func_ptr) error_cb )
{
    static off_t lastoffset = 0;
    ssize_t nread = 0;
    if ( offset != lastoffset )
    {
        error_cb( file, EXR_DEF(ERR_READ_IO), "Unable to seek in stdin stream" );
        return -1;
    }
    nread = read( STDIN_FILENO, buffer, sz );
    if ( nread > 0 )
        lastoffset = offset + nread;
    return nread;
}


static int process_stdin( int verbose )
{
    int rv;
    EXR_TYPE(FILE) *e = NULL;

    rv = EXR_FUN(start_read_stream)( &e, "<stdin>", NULL, stdin_reader, NULL, NULL, error_handler_cb );
	if ( rv == 0 )
	{
        EXR_FUN(print_info)( e, verbose );
		EXR_FUN(close)( &e );
    }
    return rv;
}

static int process_file( const char *filename, int verbose )
{
    int rv;
    EXR_TYPE(FILE) *e = NULL;

    rv = EXR_FUN(start_read)( &e, filename, error_handler_cb );
	if ( rv == 0 )
	{
        EXR_FUN(print_info)( e, verbose );
		EXR_FUN(close)( &e );
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
