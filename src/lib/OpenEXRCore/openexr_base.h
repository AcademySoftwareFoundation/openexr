/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_BASE_H
#define OPENEXR_BASE_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Retrieve the current library version. The 'extra' string is for
 *  custom installs, and is a static string, do not free the returned pointer */
EXR_EXPORT void EXR_FUN(get_library_version)( int *maj, int *min, int *patch, const char **extra );

/** Opaque file handle
 *
 * The implementation of this is 'hidden', all accesses to relevant
 * file data should happen using provided functions for version
 * portability. This handle serves as a container and identifier
 * for all the metadata and parts associated with a file
 */
typedef struct EXR_TYPE(file) EXR_TYPE(FILE);

/** 
 * @defgroup error related definitions
 * @brief These are a group of definitons related to error handling
 * @{
 */

/** error codes that may be returned by various functions */
typedef enum
{
    EXR_DEF(ERR_SUCCESS) = 0,
    EXR_DEF(ERR_OUT_OF_MEMORY),
    EXR_DEF(ERR_INVALID_ARGUMENT),
    EXR_DEF(ERR_FILE_ACCESS),
    EXR_DEF(ERR_FILE_BAD_HEADER),
    EXR_DEF(ERR_NOT_OPEN_READ),
    EXR_DEF(ERR_NOT_OPEN_WRITE),
    EXR_DEF(ERR_READ_IO),
    EXR_DEF(ERR_WRITE_IO),
    EXR_DEF(ERR_NAME_TOO_LONG),
    EXR_DEF(ERR_MISSING_REQ_ATTR),
    EXR_DEF(ERR_INVALID_ATTR),
    EXR_DEF(ERR_BAD_CHUNK_DATA),
    EXR_DEF(ERR_TYPE_MISMATCH),
    EXR_DEF(ERR_SIZE_MISMATCH),
    EXR_DEF(ERR_SCAN_TILE_MIXEDAPI),
    EXR_DEF(ERR_TILE_SCAN_MIXEDAPI),
    EXR_DEF(ERR_UNKNOWN)
} EXR_TYPE(ERROR_CODES);

/** Stream error notifier
 *
 *  This is provided to the stream functions so they can provide a nice error message to the user
 *  during stream operations
 */
typedef int (* EXR_TYPE(stream_error_func_ptr) )( EXR_TYPE(FILE) *file, int code, const char *fmt, ... ) EXR_PRINTF_FUNC_ATTRIBUTE;

/** Error callback function
 *
 *  Because a file can be read from using many threads at once, it is
 *  difficult to store an error message for later retrieval. As such,
 *  when a file is constructed, a callback function can be provided
 *  which delivers an error message for the calling application to
 *  handle. This will be delivered on the same thread causing the
 *  error.
 */
typedef void (* EXR_TYPE(error_handler_cb) )( EXR_TYPE(FILE) *file, int code, const char *msg );

/** @brief Returns a string corresponding to the specified error code. */
EXR_EXPORT const char *EXR_FUN(get_default_error_message)( int code );

/** @brief Limit the size of image allowed to be created by the library */
EXR_EXPORT void EXR_FUN(set_maximum_image_size)( int w, int h );
/** @brief Maximum width of an data window allowed to be parsed by the library */
EXR_EXPORT int EXR_FUN(get_maximum_image_width)();
/** @brief Maximum height of an data window allowed to be parsed by the library */
EXR_EXPORT int EXR_FUN(get_maximum_image_height)();

/** @brief Limit the size of an image tile allowed to be created by the library */
EXR_EXPORT void EXR_FUN(set_maximum_tile_size)( int w, int h );
/** @brief Maximum width of an image tile allowed to be parsed by the library */
EXR_EXPORT int EXR_FUN(get_maximum_tile_width)();
/** @brief Maximum height of an image tile allowed to be parsed by the library */
EXR_EXPORT int EXR_FUN(get_maximum_tile_height)();

/** @} */

/** 
 * @defgroup memory related functions
 * @brief These are a group of definitons related to memory handling
 * @{
 */

/** @brief function pointer used to hold a malloc-like routine */
typedef void *(* EXR_TYPE(memory_allocation_func) )( size_t bytes );
/** @brief function pointer used to hold a free-like routine */
typedef void (* EXR_TYPE(memory_free_func) )( void *ptr );


/** @brief Allows the user to override internal allocations necessary for
 * files, attributes, and other temporary memory.
 *
 * TODO: Should this be customizeable per file handle instead of global?
 */
EXR_EXPORT void EXR_FUN(set_memory_routines)(
    EXR_TYPE(memory_allocation_func) alloc_func,
    EXR_TYPE(memory_free_func) free_func );

/** @} */

/**************************************/

/** Debug function, prints to stdout the parts and attributes of the file passed in */
EXR_EXPORT void EXR_FUN(print_info)( EXR_TYPE(FILE) *f, int verbose );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_BASE_H */