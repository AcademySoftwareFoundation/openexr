/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_memory.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <stdlib.h>
#endif

/**************************************/

static exr_memory_allocation_func_t _glob_alloc_func = NULL;
static exr_memory_free_func_t _glob_free_func = NULL;

/**************************************/

void exr_set_default_memory_routines(
    exr_memory_allocation_func_t alloc_func,
    exr_memory_free_func_t free_func )
{
    _glob_alloc_func = alloc_func;
    _glob_free_func = free_func;
}

/**************************************/

void *internal_exr_alloc( size_t bytes )
{
    if ( _glob_alloc_func )
        return (*_glob_alloc_func)( bytes );
#ifdef _WIN32
    return HeapAlloc( GetProcessHeap(), 0, bytes );
#else
    return malloc( bytes );
#endif
}

/**************************************/

void internal_exr_free( void *ptr )
{
    if ( ! ptr )
        return;
    
    if ( _glob_free_func )
    {
        (*_glob_free_func)( ptr );
    }
    else
    {
#ifdef _WIN32
        HeapFree( GetProcessHeap(), 0, ptr );
#else
        free( ptr );
#endif
    }
}

