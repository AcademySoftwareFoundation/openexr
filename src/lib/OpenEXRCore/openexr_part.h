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
exr_part_get_count (const exr_context_t ctxt, int* count);

/** @brief Query the part name for the specified part
 *
 * NB: If this file is a single part file and name has not been set, this
 * will output NULL
 */
EXR_EXPORT exr_result_t
exr_part_get_name (const exr_context_t ctxt, int part_index, const char** out);

/** @brief Query the storage type for the specified part */
EXR_EXPORT exr_result_t exr_part_get_storage (
    const exr_context_t ctxt, int part_index, exr_storage_t* out);

/* @brief Define a new part in the file.
 *
 * If @param adopt_attr_ownership is non-zero, indicates that the
 * attribute list should be internalized and used as the attributes,
 * avoiding a number of memory copies.
 */
EXR_EXPORT exr_result_t
exr_part_add (exr_context_t ctxt, const char* partname, exr_storage_t type);

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
EXR_EXPORT exr_result_t exr_part_get_tile_levels (
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
EXR_EXPORT exr_result_t exr_part_get_tile_sizes (
    const exr_context_t ctxt,
    int                 part_index,
    int                 levelx,
    int                 levely,
    int32_t*            tilew,
    int32_t*            tileh);

/** Return the number of chunks contained in this part of the file
 *
 * This should be used as a basis for splitting up how a file is
 * processed. Depending on the compression, a different number of
 * scanlines are encoded in each chunk, and since those need to be
 * encoded / decoded as a block, the chunk should be the basis for I/O
 * as well.
 */
EXR_EXPORT exr_result_t exr_part_get_chunk_count (
    const exr_context_t ctxt, int part_index, int32_t* out);

/** Return the number of scanlines chunks for this file part
 *
 * When iterating over a scanline file, this may be an easier metric
 * for multi-threading or other access than only negotiating chunk
 * counts, and so is provided as a utility.
 */
EXR_EXPORT exr_result_t exr_part_get_scanlines_per_chunk (
    const exr_context_t ctxt, int part_index, int32_t* out);

/** returns the maximum unpacked size of a chunk for the file part.
 *
 * This may be used ahead of any actual reading of data, so can be
 * used to pre-allocate buffers for multiple threads in one block or
 * whatever your application may require.
 */
EXR_EXPORT exr_result_t exr_part_get_chunk_unpacked_size (
    const exr_context_t ctxt, int part_index, uint64_t* out);

/**************************************/

/** @defgroup PartMetadata Functions to get and set metadata for a particular part
 * @{
 *
 */

/** @brief Query the count of attributes in a part */
EXR_EXPORT exr_result_t exr_part_get_attribute_count (
    const exr_context_t ctxt, int part_index, int32_t* count);

enum exr_attr_list_access_mode
{
    EXR_ATTR_LIST_FILE_ORDER,  /**< order they appear in the file */
    EXR_ATTR_LIST_SORTED_ORDER /**< alphabetically sorted */
};

/** @brief Query a particular attribute by index */
EXR_EXPORT exr_result_t exr_part_get_attribute_by_index (
    const exr_context_t            ctxt,
    int                            part_index,
    enum exr_attr_list_access_mode mode,
    int32_t                        idx,
    const exr_attribute_t**        outattr);

/** @brief Query a particular attribute by name */
EXR_EXPORT exr_result_t exr_part_get_attribute_by_name (
    const exr_context_t     ctxt,
    int                     part_index,
    const char*             name,
    const exr_attribute_t** outattr);

/** @brief Query the list of attributes in a part
 *
 * This retrieves a list of attributes currently defined in a part
 *
 * if outlist is NULL, this function still succeeds, filling only the
 * count. In this manner, the user can allocate memory for the list of
 * attributes, then re-call this function to get the full list
 */
EXR_EXPORT exr_result_t exr_part_get_attribute_list (
    const exr_context_t            ctxt,
    int                            part_index,
    enum exr_attr_list_access_mode mode,
    int32_t*                       count,
    const exr_attribute_t**        outlist);

/** Declare an attribute within the specified part.
 *
 * Only valid when a file is opened for write.
 */
EXR_EXPORT exr_result_t exr_part_attr_declare_by_type (
    exr_context_t     ctxt,
    int               part_index,
    const char*       name,
    const char*       type,
    exr_attribute_t** newattr);

/** @brief Declare an attribute within the specified part.
 *
 * Only valid when a file is opened for write.
 */
EXR_EXPORT exr_result_t exr_part_attr_declare (
    exr_context_t        ctxt,
    int                  part_index,
    const char*          name,
    exr_attribute_type_t type,
    exr_attribute_t**    newattr);

/** 
 * @defgroup RequiredAttributeHelpers Required Attribute Utililities
 *
 * @brief These are a group of functions for attributes that are
 * required to be in every part of every file.
 *
 * @{
 */

/** @brief initialize all required attributes for all files.
 *
 * NB: other file types do require other attributes, such as the tile
 * description for a tiled file
 */
EXR_EXPORT exr_result_t exr_part_initialize_required_attr (
    exr_context_t           ctxt,
    int                     part_index,
    const exr_attr_box2i_t* displayWindow,
    const exr_attr_box2i_t* dataWindow,
    float                   pixelaspectratio,
    const exr_attr_v2f_t*   screenWindowCenter,
    float                   screenWindowWidth,
    exr_lineorder_t         lineorder,
    exr_compression_t       ctype);

/** @brief initializes all required attributes to default values
 *
 * displayWindow is set to (0, 0 -> width - 1, height - 1)
 * dataWindow is set to (0, 0 -> width - 1, height - 1)
 * pixelAspectRatio is set to 1.0
 * screenWindowCenter is set to 0.f, 0.f
 * screenWindowWidth is set to 1.f
 * lineorder is set to INCREASING_Y
 * compression is set to @param ctype
 */
EXR_EXPORT exr_result_t exr_part_initialize_required_attr_simple (
    exr_context_t     ctxt,
    int               part_index,
    int32_t           width,
    int32_t           height,
    exr_compression_t ctype);

/** @brief retrieves the list of channels */
EXR_EXPORT exr_result_t exr_part_get_channels (
    const exr_context_t ctxt, int part_index, const exr_attr_chlist_t** chlist);

/** @brief Defines a new channel to the output file part. */
EXR_EXPORT int exr_part_add_channel (
    exr_context_t    ctxt,
    int              part_index,
    const char*      name,
    exr_pixel_type_t ptype,
    uint8_t          islinear,
    int32_t          xsamp,
    int32_t          ysamp);

/** @brief Copies the channels from another source.
 *
 * Useful if you are manually constructing the list or simply copying
 * from an input file */
EXR_EXPORT exr_result_t exr_part_set_channels (
    exr_context_t ctxt, int part_index, const exr_attr_chlist_t* channels);

/** @brief Retrieves the compression method used for the specified part. */
EXR_EXPORT exr_result_t exr_part_get_compression (
    const exr_context_t ctxt, int part_index, exr_compression_t* compression);
/** @brief Sets the compression method used for the specified part. */
EXR_EXPORT exr_result_t exr_part_set_compression (
    exr_context_t ctxt, int part_index, exr_compression_t ctype);

/** @brief Retrieves the data window for the specified part. */
EXR_EXPORT exr_result_t exr_part_get_data_window (
    const exr_context_t ctxt, int part_index, exr_attr_box2i_t* out);
/** @brief Sets the data window for the specified part. */
EXR_EXPORT int exr_part_set_data_window (
    exr_context_t ctxt, int part_index, const exr_attr_box2i_t* dw);

/** @brief Retrieves the display window for the specified part. */
EXR_EXPORT exr_result_t exr_part_get_display_window (
    const exr_context_t ctxt, int part_index, exr_attr_box2i_t* out);
/** @brief Sets the display window for the specified part. */
EXR_EXPORT int exr_part_set_display_window (
    exr_context_t ctxt, int part_index, const exr_attr_box2i_t* dw);

/** @brief Retrieves the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_get_lineorder (
    const exr_context_t ctxt, int part_index, exr_lineorder_t* out);
/** @brief Sets the line order for storing data in the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_set_lineorder (
    exr_context_t ctxt, int part_index, exr_lineorder_t lo);

/** @brief Retrieves the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_get_pixel_aspect_ratio (
    const exr_context_t ctxt, int part_index, float* par);
/** @brief Sets the pixel aspect ratio for the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t
exr_part_set_pixel_aspect_ratio (exr_context_t ctxt, int part_index, float par);

/** @brief Retrieves the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_get_screen_window_center (
    const exr_context_t ctxt, int part_index, exr_attr_v2f_t* wc);
/** @brief Sets the screen oriented window center for the specified part (use 0 for single part images). */
EXR_EXPORT int exr_part_set_screen_window_center (
    exr_context_t ctxt, int part_index, const exr_attr_v2f_t* wc);

/** @brief Retrieves the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_get_screen_window_width (
    const exr_context_t ctxt, int part_index, float* out);
/** @brief Sets the screen oriented window width for the specified part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_set_screen_window_width (
    exr_context_t ctxt, int part_index, float ssw);

/** @brief Retrieves the tiling info for a tiled part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_get_tile_descriptor (
    const exr_context_t    ctxt,
    int                    part_index,
    uint32_t*              xsize,
    uint32_t*              ysize,
    exr_tile_level_mode_t* level,
    exr_tile_round_mode_t* round);

/** @brief Sets the tiling info for a tiled part (use 0 for single part images). */
EXR_EXPORT exr_result_t exr_part_set_tile_descriptor (
    exr_context_t         ctxt,
    int                   part_index,
    uint32_t              x_size,
    uint32_t              y_size,
    exr_tile_level_mode_t level_mode,
    exr_tile_round_mode_t round_mode);

EXR_EXPORT exr_result_t exr_part_set_name (
    exr_context_t ctxt, int part_index, const char *val);

EXR_EXPORT exr_result_t exr_part_get_version (
    const exr_context_t ctxt, int part_index, int32_t *out);

EXR_EXPORT exr_result_t exr_part_set_version (
    exr_context_t ctxt, int part_index, int32_t val);

EXR_EXPORT exr_result_t exr_part_set_chunk_count (
    exr_context_t ctxt, int part_index, int32_t val);

/** @} */ /* required attr group */

/** 
 * @defgroup BuiltinAttributeHelpers Attribute Utililities for builtin types
 *
 * @brief These are a group of functions for attributes that use the builtin types
 *
 * @{
 */

EXR_EXPORT exr_result_t exr_part_attr_get_box2i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_box2i_t*   outval);

EXR_EXPORT exr_result_t exr_part_attr_set_box2i (
    exr_context_t           ctxt,
    int                     part_index,
    const char*             name,
    const exr_attr_box2i_t* val);

EXR_EXPORT exr_result_t exr_part_attr_get_box2f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_box2f_t*   outval);

EXR_EXPORT exr_result_t exr_part_attr_set_box2f (
    exr_context_t           ctxt,
    int                     part_index,
    const char*             name,
    const exr_attr_box2f_t* val);

/** @brief zero copy query of channel data.
 *
 *
 * Do not free or manipulate the const exr_attr_chlist_t data, or use
 * after the lifetime of the context
 */
EXR_EXPORT exr_result_t exr_part_attr_get_channels (
    const exr_context_t       ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_chlist_t** chlist);

EXR_EXPORT exr_result_t exr_part_attr_set_channels (
    exr_context_t            ctxt,
    int                      part_index,
    const char*              name,
    const exr_attr_chlist_t* channels);

EXR_EXPORT exr_result_t exr_part_attr_get_chromaticities (
    const exr_context_t        ctxt,
    int                        part_index,
    const char*                name,
    exr_attr_chromaticities_t* chroma);

EXR_EXPORT exr_result_t exr_part_attr_set_chromaticities (
    exr_context_t                    ctxt,
    int                              part_index,
    const char*                      name,
    const exr_attr_chromaticities_t* chroma);

EXR_EXPORT exr_result_t exr_part_attr_get_compression (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_compression_t*  out);

EXR_EXPORT exr_result_t exr_part_attr_set_compression (
    exr_context_t     ctxt,
    int               part_index,
    const char*       name,
    exr_compression_t comp);

EXR_EXPORT exr_result_t exr_part_attr_get_double (
    const exr_context_t ctxt, int part_index, const char* name, double* out);

EXR_EXPORT exr_result_t exr_part_attr_set_double (
    exr_context_t ctxt, int part_index, const char* name, double val);

EXR_EXPORT exr_result_t exr_part_attr_get_envmap (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_envmap_t*       out);

EXR_EXPORT exr_result_t exr_part_attr_set_envmap (
    exr_context_t ctxt, int part_index, const char* name, exr_envmap_t emap);

EXR_EXPORT exr_result_t exr_part_attr_get_float (
    const exr_context_t ctxt, int part_index, const char* name, float* out);

EXR_EXPORT exr_result_t exr_part_attr_set_float (
    exr_context_t ctxt, int part_index, const char* name, float val);

/** @brief zero copy query of float data.
 *
 * Do not free or manipulate the const float * data, or use after the
 * lifetime of the context
 */
EXR_EXPORT exr_result_t exr_part_attr_get_float_vector (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            sz,
    const float**       out);

EXR_EXPORT exr_result_t exr_part_attr_set_float_vector (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    int32_t       sz,
    const float*  vals);

EXR_EXPORT exr_result_t exr_part_attr_get_int (
    const exr_context_t ctxt, int part_index, const char* name, int32_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_int (
    exr_context_t ctxt, int part_index, const char* name, int32_t val);

EXR_EXPORT exr_result_t exr_part_attr_get_keycode (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_keycode_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_keycode (
    exr_context_t             ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_keycode_t* kc);

EXR_EXPORT exr_result_t exr_part_attr_get_lineorder (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_lineorder_t*    out);

EXR_EXPORT exr_result_t exr_part_attr_set_lineorder (
    exr_context_t ctxt, int part_index, const char* name, exr_lineorder_t lo);

EXR_EXPORT exr_result_t exr_part_attr_get_m33f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m33f_t*    out);

EXR_EXPORT exr_result_t exr_part_attr_set_m33f (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m33f_t* m);

EXR_EXPORT exr_result_t exr_part_attr_get_m33d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m33d_t*    out);

EXR_EXPORT exr_result_t exr_part_attr_set_m33d (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m33d_t* m);

EXR_EXPORT exr_result_t exr_part_attr_get_m44f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m44f_t*    out);

EXR_EXPORT exr_result_t exr_part_attr_set_m44f (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m44f_t* m);

EXR_EXPORT exr_result_t exr_part_attr_get_m44d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_m44d_t*    out);

EXR_EXPORT exr_result_t exr_part_attr_set_m44d (
    exr_context_t          ctxt,
    int                    part_index,
    const char*            name,
    const exr_attr_m44d_t* m);

EXR_EXPORT exr_result_t exr_part_attr_get_preview (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_preview_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_preview (
    exr_context_t             ctxt,
    int                       part_index,
    const char*               name,
    const exr_attr_preview_t* p);

EXR_EXPORT exr_result_t exr_part_attr_get_rational (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_rational_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_rational (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_rational_t* r);

/** @brief zero copy query of string value.
 *
 * Do not modify the string pointed to by @param out, and do not use
 * after the lifetime of the context
 */
EXR_EXPORT exr_result_t exr_part_attr_get_string (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            length,
    const char**        out);

EXR_EXPORT exr_result_t exr_part_attr_set_string (
    exr_context_t ctxt, int part_index, const char* name, const char* s);

/** @brief zero copy query of string data.
 *
 * Do not free the strings pointed to by the array.
 *
 * Must provide @param size
 *
 * @param out must be a const char** array large enough to hold the
 * string pointers for the string vector when provided
 */
EXR_EXPORT exr_result_t exr_part_attr_get_string_vector (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    int32_t*            size,
    const char**        out);

EXR_EXPORT exr_result_t exr_part_attr_set_string_vector (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    int32_t       size,
    const char**  sv);

EXR_EXPORT exr_result_t exr_part_attr_get_tiledesc (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_tiledesc_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_tiledesc (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_tiledesc_t* td);

EXR_EXPORT exr_result_t exr_part_attr_get_timecode (
    const exr_context_t  ctxt,
    int                  part_index,
    const char*          name,
    exr_attr_timecode_t* out);

EXR_EXPORT exr_result_t exr_part_attr_set_timecode (
    exr_context_t              ctxt,
    int                        part_index,
    const char*                name,
    const exr_attr_timecode_t* tc);

EXR_EXPORT exr_result_t exr_part_attr_get_v2i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2i_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v2i (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2i_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_v2f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2f_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v2f (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2f_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_v2d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v2d_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v2d (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v2d_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_v3i (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3i_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v3i (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3i_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_v3f (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3f_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v3f (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3f_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_v3d (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    exr_attr_v3d_t*     out);

EXR_EXPORT exr_result_t exr_part_attr_set_v3d (
    exr_context_t         ctxt,
    int                   part_index,
    const char*           name,
    const exr_attr_v3d_t* v);

EXR_EXPORT exr_result_t exr_part_attr_get_user (
    const exr_context_t ctxt,
    int                 part_index,
    const char*         name,
    const char**        type,
    int32_t*            size,
    const void**        out);

EXR_EXPORT exr_result_t exr_part_attr_set_user (
    exr_context_t ctxt,
    int           part_index,
    const char*   name,
    const char*   type,
    int32_t       size,
    const void*   out);

/** @} */ /* built-in attr group */

/** @} */ /* metadata group */

/** @} */ /* part group */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_PART_H */
