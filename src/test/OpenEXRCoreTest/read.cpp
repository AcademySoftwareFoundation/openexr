// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenEXR Project.

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include "read.h"

#include <openexr.h>

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <iostream>

static void err_cb( exr_file_t *f, int code, const char *msg )
{
    std::cerr << "err_cb ERROR " << code << ": " << msg << std::endl;
}

ssize_t dummyreadstream( exr_file_t *f, void *, void *, size_t, off_t,
                         exr_stream_error_func_ptr_t errcb )
{
    return -1;
}

void testReadBadArgs( const std::string &tempdir )
{
    exr_file_t *f;
    std::string fn = tempdir + "invalid.exr";
    assert(EXR_ERR_INVALID_ARGUMENT == exr_start_read( NULL, fn.c_str(), NULL ) );
    assert(EXR_ERR_INVALID_ARGUMENT == exr_start_read( &f, NULL, NULL ) );
    assert(EXR_ERR_INVALID_ARGUMENT == exr_start_read( &f, NULL, &err_cb ) );

    assert(EXR_ERR_INVALID_ARGUMENT == exr_start_read_stream(
               &f, fn.c_str(), NULL, NULL, NULL, NULL, NULL ) );
    assert(EXR_ERR_INVALID_ARGUMENT == exr_start_read_stream(
               NULL, fn.c_str(), NULL, &dummyreadstream, NULL, NULL, NULL ) );

    assert(EXR_ERR_FILE_ACCESS == exr_start_read(
               &f, fn.c_str(), &err_cb ) );
}

void testReadBadFiles( const std::string &tempdir )
{
    exr_file_t *f;
    std::string fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "invalid.exr";
    int rv;
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv != EXR_ERR_SUCCESS );
}

void testOpenScans( const std::string &tempdir )
{
    exr_file_t *f;
    std::string fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.1.exr";
    int rv;
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );
    exr_close( &f );

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.planar.exr";
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );
    exr_close( &f );

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.interleaved.exr";
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );
    exr_close( &f );
}

void testOpenTiles( const std::string &tempdir )
{
    exr_file_t *f;
    std::string fn = ILM_IMF_TEST_IMAGEDIR;

    fn += "tiled.exr";
    int rv;
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );
    exr_close( &f );

    fn = ILM_IMF_TEST_IMAGEDIR;
    fn += "v1.7.test.tiled.exr";
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );
    exr_close( &f );

}

void testOpenMultiPart( const std::string &tempdir )
{
}

void testOpenDeep( const std::string &tempdir )
{
}

void testReadScans( const std::string &tempdir )
{
}

void testReadTiles( const std::string &tempdir )
{
    exr_file_t *f;
    std::string fn = ILM_IMF_TEST_IMAGEDIR;

    fn += "v1.7.test.tiled.exr";
    int rv;
    rv = exr_start_read( &f, fn.c_str(), &err_cb );
    assert( rv == EXR_ERR_SUCCESS );

    assert( EXR_STORAGE_TILED == exr_get_part_storage( f, 0 ) );

    int levelsx = -1, levelsy = -1;
    rv = exr_get_tile_levels( NULL, 0, &levelsx, &levelsy );
    assert( rv == -1 );
    assert( levelsx == -1 );
    assert( levelsy == -1 );

    rv = exr_get_tile_levels( f, 1, &levelsx, &levelsy );
    assert( rv == EXR_ERR_INVALID_ARGUMENT );
    assert( levelsx == -1 );
    assert( levelsy == -1 );

    rv = exr_get_tile_levels( f, 0, NULL, NULL );
    assert( rv == EXR_ERR_SUCCESS );

    levelsx = -1;
    rv = exr_get_tile_levels( f, 0, &levelsx, NULL );
    assert( rv == EXR_ERR_SUCCESS );
    assert( levelsx == 1 );

    levelsy = -1;
    rv = exr_get_tile_levels( f, 0, NULL, &levelsy );
    assert( rv == EXR_ERR_SUCCESS );
    assert( levelsy == 1 );

    levelsx = levelsy = -1;
    rv = exr_get_tile_levels( f, 0, &levelsx, &levelsy );
    assert( rv == EXR_ERR_SUCCESS );
    assert( levelsx == 1 );
    assert( levelsy == 1 );

    exr_decode_chunk_info_t chunk = {0};
    rv = exr_decode_chunk_init_scanline( f, 0, &chunk, 42, 1 );
    assert( rv == EXR_ERR_SCAN_TILE_MIXEDAPI );

    // actually read a tile...
    rv = exr_decode_chunk_init_tile( f, 0, &chunk, 1, 1, 0, 0, 1 );
    assert( rv == EXR_ERR_SUCCESS );
    assert( chunk.unpacked.size == exr_get_chunk_unpacked_size( f, 0 ) );

    rv = exr_read_chunk( f, &chunk );
    assert( rv == EXR_ERR_SUCCESS );
    assert( chunk.packed.buffer != NULL );
    assert( chunk.unpacked.buffer != NULL );

    exr_destroy_decode_chunk_info( &chunk );
    exr_close( &f );

}

void testReadMultiPart( const std::string &tempdir )
{
}

void testReadDeep( const std::string &tempdir )
{
}

