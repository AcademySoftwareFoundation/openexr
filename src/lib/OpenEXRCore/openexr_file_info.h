/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_QUERY_H
#define OPENEXR_CORE_QUERY_H

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup Generic File Query Functions
 * @brief These are a group of functions used when both reading and writing files
 * @{
 */

/** Returns either the raw file name or the provided stream file name depending
 *  on which file creation routine was used
 */
EXR_EXPORT const char *exr_get_file_name( exr_file_t *file );

/** @brief Query how many parts are in the file, returns 0 on error */
EXR_EXPORT int exr_get_part_count( exr_file_t *file );

/** @brief Query whether the file has deep (nonimage data) in at least
 * one of it's constituent parts */
EXR_EXPORT int exr_has_deep( exr_file_t *file );

/** @brief Query the part name for the specified part
 *
 * NB: If this file is a single part file and name has not been set, this
 * will return NULL
 */
EXR_EXPORT const char * exr_get_part_name(
    exr_file_t *file,
    int part_index );

/** @brief Query the storage type for the specified part */
EXR_EXPORT exr_STORAGE_TYPE_t exr_get_part_storage(
    exr_file_t *file,
    int part_index );

/** @brief Query how many levels are in the specified part.
 *
 * If the part is a tiled part, fills in how many tile levels are present.
 *
 * return ERR_SUCCESS on sucess, an error otherwise (i.e. if the part
 * is not tiled)
 *
 * it is valid to pass NULL to either of the levelsx or levelsy
 * arguments, which enables testing if this part is a tiled part, or
 * if you don't need both (i.e. in the case of a mip-level tiled
 * image)
 */
EXR_EXPORT int exr_get_tile_levels( exr_file_t *file, int part_index, int32_t *levelsx, int32_t *levelsy );

/** Return the number of chunks contained in this part of the file
 *
 * This should be used as a basis for splitting up how a file is
 * read. Depending on the compression, a different number of scanlines
 * are encoded in each chunk, and since those need to be encoded /
 * decoded as a block, the chunk should be the basis for I/O as well.
 * 
 * @return -1 on failure
 */
EXR_EXPORT int32_t exr_get_chunk_count( exr_file_t *file, int part_index );

/** Return the number of scanlines chunks for this file part
 *
 * When iterating over a scanline file, this may be an easier metric
 * than only negotiating chunk counts, and so is provided as a utility.
 * 
 * @return -1 on failure
 */
EXR_EXPORT int32_t exr_get_scanlines_per_chunk( exr_file_t *file, int part_index );

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_QUERY_H */
