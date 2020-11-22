/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_priv_memory.h"

#ifdef _WIN32
#  include <heapapi.h>
#else
#  include <stdlib.h>
#endif

/**************************************/

static EXR_TYPE(memory_allocation_func) _glob_alloc_func = NULL;
static EXR_TYPE(memory_free_func) _glob_free_func = NULL;
#ifdef _WIN32
static HANDLE _glob_heap = GetProcessHeap();
#endif

/**************************************/

void EXR_FUN(set_memory_routines)(
    EXR_TYPE(memory_allocation_func) alloc_func,
    EXR_TYPE(memory_free_func) free_func )
{
    _glob_alloc_func = alloc_func;
    _glob_free_func = free_func;
}

/**************************************/

void *priv_alloc( size_t bytes )
{
    if ( _glob_alloc_func )
        return (*_glob_alloc_func)( bytes );
#ifdef _WIN32
    return HeapAlloc( _glob_heap, 0, bytes );
#else
    return malloc( bytes );
#endif
}

/**************************************/

void priv_free( void *ptr )
{
    if ( ! ptr )
        return;
    
    if ( _glob_free_func )
        return (*_glob_free_func)( ptr );
#ifdef _WIN32
    return HeapFree( _glob_heap, 0, ptr );
#else
    return free( ptr );
#endif
}

