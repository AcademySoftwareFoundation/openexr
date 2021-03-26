/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PART_H
#define OPENEXR_PART_H

#include "openexr_context.h"

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup PartInfo Part related definitions
 *
 * A part is a separate entity in the OpenEXR file. This was
 * formalized in the OpenEXR 2.0 timeframe to allow there to be a
 * clear set of eyes for stereo, or just a simple list of AOVs within
 * a single OpenEXR file. Prior, it was managed by name convention,
 * but with a multi-part file, they are clearly separate types, and
 * can have separate behavior.
 *
 * This is a set of functions to query, or set up when writing, that
 * set of parts within a file. This remains backward compatible to
 * OpenEXR files from before this change, in that a file with a single
 * part is a subset of a multi-part file. As a special case, creating
 * a file with a single part will write out as if it is a file which
 * is not multi-part aware, so as to be compatible with those old
 * libraries.
 *
 * @{
 */

/** @brief Query how many parts are in the file */
EXR_EXPORT exr_result_t
exr_get_part_count (const exr_context_t ctxt, int* count);

/** @brief Query the part name for the specified part
 *
 * NB: If this file is a single part file and name has not been set, this
 * will output NULL
 */
EXR_EXPORT exr_result_t
exr_get_part_name (const exr_context_t ctxt, int part_index, const char** out);

/** @brief Query the storage type for the specified part */
EXR_EXPORT exr_result_t exr_get_part_storage (
    const exr_context_t ctxt, int part_index, exr_storage_t* out);

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
EXR_EXPORT exr_result_t exr_get_tile_levels (
    const exr_context_t ctxt,
    int                 part_index,
    int32_t*            levelsx,
    int32_t*            levelsy);

/** @brief Query the tile size for a particular level in the specified part.
 *
 * If the part is a tiled part, fills in the tile size for the specified part / level
 *
 * return ERR_SUCCESS on sucess, an error otherwise (i.e. if the part
 * is not tiled)
 *
 * it is valid to pass NULL to either of the tilew or tileh
 * arguments, which enables testing if this part is a tiled part, or
 * if you don't need both (i.e. in the case of a mip-level tiled
 * image)
 */
EXR_EXPORT exr_result_t exr_get_tile_sizes (
    const exr_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            tilew,
    int32_t*            tileh);

/** Return the number of chunks contained in this part of the file
 *
 * This should be used as a basis for splitting up how a file is
 * read. Depending on the compression, a different number of scanlines
 * are encoded in each chunk, and since those need to be encoded /
 * decoded as a block, the chunk should be the basis for I/O as well.
 */
EXR_EXPORT exr_result_t
exr_get_chunk_count (const exr_context_t ctxt, int part_index, int32_t* out);

/** Return the number of scanlines chunks for this file part
 *
 * When iterating over a scanline file, this may be an easier metric
 * than only negotiating chunk counts, and so is provided as a utility.
 */
EXR_EXPORT exr_result_t exr_get_scanlines_per_chunk (
    const exr_context_t ctxt, int part_index, int32_t* out);

/** @defgroup PartMetadata Functions to get and set metadata for a particular part
 * @{
 *
 */

EXR_EXPORT exr_result_t exr_attr_list_set_box2i(
    exr_context_t ctxt,
    int part_index,
    const char *name,
    const exr_attr_box2i_t *val );

EXR_EXPORT exr_result_t exr_attr_list_get_box2i(
    const exr_context_t ctxt,
    int part_index,
    const char *name,
    exr_attr_box2i_t *outval );

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_PART_H */
