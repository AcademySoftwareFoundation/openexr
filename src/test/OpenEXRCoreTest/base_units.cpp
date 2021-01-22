/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <openexr.h>

#include "base_units.h"

#include <assert.h>
#include <iostream>
#include <cstring>

void testBase( const std::string &tempdir )
{
    int maj, min, patch;
    const char *extra;
    const char *compextra = COMP_EXTRA;

    exr_get_library_version( &maj, &min, &patch, &extra );
    if ( maj != COMP_MAJ ||
         min != COMP_MIN ||
         patch != COMP_PATCH ||
         !strcmp( extra, compextra ) )
    {
        std::cerr << "ERROR testing library, wrong library version: "
                  << maj << "." << min << "." << patch;
        if ( extra[0] != '\0' )
            std::cerr << "-" << extra;
        std::cerr << " vs compiled in "
                  << COMP_MAJ << "." << COMP_MIN << "." << COMP_PATCH;
        if ( compextra[0] != '\0' )
            std::cerr << "-" << compextra;
        std::cerr << std::endl;
        assert(false);
    }
    std::cout << "Testing OpenEXR library version: "
                  << maj << "." << min << "." << patch;
    if ( extra[0] != '\0' )
            std::cout << "-" << extra;
    std::cout << std::endl;

    exr_get_library_version( NULL, &min, &patch, &extra );
    exr_get_library_version( &maj, NULL, &patch, &extra );
    exr_get_library_version( &maj, &min, NULL, &extra );
    exr_get_library_version( &maj, &min, &patch, NULL );
}

void testBaseErrors( const std::string &tempdir )
{
    const char *errmsg;

    // just spot check that we get results and don't get null for out
    // of bounds access
    errmsg = exr_get_default_error_message( EXR_ERR_SUCCESS );
    if ( errmsg == NULL )
    {
        assert(false);
    }
    errmsg = exr_get_default_error_message( EXR_ERR_OUT_OF_MEMORY );
    if ( errmsg == NULL )
    {
        assert(false);
    }
    errmsg = exr_get_default_error_message( EXR_ERR_UNKNOWN );
    if ( errmsg == NULL )
    {
        assert(false);
    }
    errmsg = exr_get_default_error_message( -1 );
    if ( errmsg == NULL || strcmp( errmsg, "Unable to allocate memory" ) )
    {
        std::cerr << "errmsg: " << errmsg << std::endl;
        assert(false);
    }
    errmsg = exr_get_default_error_message( (int)EXR_ERR_UNKNOWN + 1 );
    if ( errmsg == NULL )
    {
        assert(false);
    }
    errmsg = exr_get_default_error_message( 110 );
    if ( errmsg == NULL || strcmp( errmsg, "Unknown error code" ) )
    {
        assert(false);
    }
}

void testBaseLimits( const std::string &tempdir )
{
    exr_set_maximum_image_size( 42, 42 );
    if ( exr_get_maximum_image_width() != 42 ||
         exr_get_maximum_image_height() != 42 )
    {
        std::cerr << "Unable to set_maximum_image_size: 42, 42 -> "
                  << exr_get_maximum_image_width() << ", "
                  << exr_get_maximum_image_height() << std::endl;
        assert(false);
    }
    exr_set_maximum_image_size( -1, -1 );
    if ( exr_get_maximum_image_width() != 42 ||
         exr_get_maximum_image_height() != 42 )
    {
        std::cerr << "Invalid request not ignored to set_maximum_image_size: 42, 42 -> "
                  << exr_get_maximum_image_width() << ", "
                  << exr_get_maximum_image_height() << std::endl;
        assert(false);
    }

    exr_set_maximum_tile_size( 128, 128 );
    if ( exr_get_maximum_tile_width() != 128 ||
         exr_get_maximum_tile_height() != 128 )
    {
        std::cerr << "Unable to set_maximum_tile_size: 128, 128 -> "
                  << exr_get_maximum_tile_width() << ", "
                  << exr_get_maximum_tile_height() << std::endl;
        assert(false);
    }
    exr_set_maximum_tile_size( -1, -1 );
    if ( exr_get_maximum_tile_width() != 128 ||
         exr_get_maximum_tile_height() != 128 )
    {
        std::cerr << "Invalid request not ignored to set_maximum_image_size: 128, 128 -> "
                  << exr_get_maximum_tile_width() << ", "
                  << exr_get_maximum_tile_height() << std::endl;
        assert(false);
    }
    exr_set_maximum_image_size( 0, 0 );
    exr_set_maximum_tile_size( 0, 0 );
}

void testBaseDebug( const std::string &tempdir )
{
    exr_file_t *f = NULL;
    // make sure we don't crash with null file handle (there should be error prints)
    exr_print_info( NULL, 0 );
    exr_print_info( f, 0 );
}
