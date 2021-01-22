/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_H
#define OPENEXR_ATTR_H

#include "openexr_base.h"
#include "openexr_attr_simple.h"
#include "openexr_attr_tiledesc.h"
#include "openexr_attr_string.h"
#include "openexr_attr_string_vector.h"
#include "openexr_attr_float_vector.h"
#include "openexr_attr_chlist.h"
#include "openexr_attr_preview.h"
#include "openexr_attr_opaque.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup attribute list storage and functions
 * @brief These are a group of types and functions for defining an attribute, and lists of attributes
 *
 * @{
 */

/** built-in / native attribute type enum */
typedef enum
{
    EXR_ATTR_UNKNOWN = 0, /**< type indicating an error or uninitialized attribute */
    EXR_ATTR_BOX2I, /**< integer region definition. @see exr_box2i */
    EXR_ATTR_BOX2F, /**< float region definition. @see exr_box2f */
    EXR_ATTR_CHLIST, /**< Definition of channels in file @see exr_chlist_entry */
    EXR_ATTR_CHROMATICITIES, /**< Values to specify color space of colors in file @see exr_chromaticities */
    EXR_ATTR_COMPRESSION, /**< uint8_t declaring compression present */
    EXR_ATTR_DOUBLE, /**< double precision floating point number */
    EXR_ATTR_ENVMAP, /**< uint8_t declaring environment map type */
    EXR_ATTR_FLOAT, /**< a normal (4 byte) precision floating point number */
    EXR_ATTR_FLOAT_VECTOR, /**< a list of normal (4 byte) precision floating point numbers */
    EXR_ATTR_INT, /**< a 32-bit signed integer value */
    EXR_ATTR_KEYCODE, /**< structure recording keycode @see exr_keycode */
    EXR_ATTR_LINEORDER, /**< uint8_t declaring scanline ordering */
    EXR_ATTR_M33F, /**< 9 32-bit floats representing a 3x3 matrix */
    EXR_ATTR_M33D, /**< 9 64-bit floats representing a 3x3 matrix */
    EXR_ATTR_M44F, /**< 16 32-bit floats representing a 4x4 matrix */
    EXR_ATTR_M44D, /**< 16 64-bit floats representing a 4x4 matrix */
    EXR_ATTR_PREVIEW, /**< 2 unsigned ints followed by 4 x w x h uint8_t image */
    EXR_ATTR_RATIONAL, /**< int followed by unsigned int */
    EXR_ATTR_STRING, /**< int (length) followed by char string data */
    EXR_ATTR_STRING_VECTOR, /**< 0 or more text strings (int + string). number is based on attribute size */
    EXR_ATTR_TILEDESC, /**< 2 unsigned ints xSize, ySize followed by mode */
    EXR_ATTR_TIMECODE, /**< 2 unsigned ints time and flags, user data */
    EXR_ATTR_V2I, /**< pair of 32-bit integers */
    EXR_ATTR_V2F, /**< pair of 32-bit floats */
    EXR_ATTR_V2D, /**< pair of 64-bit floats */
    EXR_ATTR_V3I, /**< set of 3 32-bit integers */
    EXR_ATTR_V3F, /**< set of 3 32-bit floats */
    EXR_ATTR_V3D, /**< set of 3 64-bit floats */
    EXR_ATTR_OPAQUE, /**< user / unknown provided type */
    EXR_ATTR_LAST_KNOWN_TYPE
} exr_ATTRIBUTE_TYPE_t;

/** storage, name and type information for an attribute */
typedef struct
{
	const char *name; /**< name of the attribute */
	const char *type_name; /**< string type name of the attribute */
	uint8_t name_length; /**< length of name string (short flag is 31 max, long allows 255) */
	uint8_t type_name_length; /**< length of type string (short flag is 31 max, long allows 255) */
    exr_ATTRIBUTE_TYPE_t type; /**< enum of the attribute type */

	/** Union of pointers of different types that can be used to type
	 * pun to an appropriate type for builtins. Do note that while
	 * this looks like a big thing, it is only the size of a single
	 * pointer.  these are all pointers into some other data block
	 * storing the value you want, with the exception of the pod types
	 * which are just put in place (i.e. small value optimization)
	 */
    union
    {
		// NB: not pointers for POD types
        uint8_t uc;
        double d;
        float f;
        int32_t i;

        exr_attr_box2i_t *box2i;
        exr_attr_box2f_t *box2f;
        exr_attr_chlist_t *chlist;
        exr_attr_chromaticities_t *chromaticities;
        exr_attr_keycode_t *keycode;
        exr_attr_float_vector_t *floatvector;
        exr_attr_m33f_t *m33f;
        exr_attr_m33d_t *m33d;
        exr_attr_m44f_t *m44f;
        exr_attr_m44d_t *m44d;
        exr_attr_preview_t *preview;
        exr_attr_rational_t *rational;
        exr_attr_string_t *string;
        exr_attr_string_vector_t *stringvector;
        exr_attr_tiledesc_t *tiledesc;
        exr_attr_timecode_t *timecode;
        exr_attr_v2i_t *v2i;
        exr_attr_v2f_t *v2f;
        exr_attr_v2d_t *v2d;
        exr_attr_v3i_t *v3i;
        exr_attr_v3f_t *v3f;
        exr_attr_v3d_t *v3d;
        exr_attr_opaquedata_t *opaque;
        uint8_t *rawptr;
    };
} exr_attribute_t;

typedef struct
{
    int num_attributes; /**< number of attribute entries in the list */
    int num_alloced; /**< allocation count. if > 0, attribute list owns pointer */
    exr_attribute_t **entries; /**< creation order list of attributes */
    exr_attribute_t **sorted_entries; /**< sorted order list of attributes for fast lookup */
} exr_attribute_list_t;

/** Frees memory for all the owned attributes in the list as well as the list itself */
EXR_EXPORT void exr_attr_list_destroy(
    exr_attribute_list_t *l );

/** Computes the number of bytes required to store this attribute list in a file */
EXR_EXPORT uint64_t exr_attr_list_compute_size(
    exr_attribute_list_t *l );

/** Finds an attribute in the list by name */
EXR_EXPORT exr_attribute_t *exr_attr_list_find_by_name(
    exr_file_t *file,
    exr_attribute_list_t *l,
    const char *name );

/** Adds a new attribute to the list with a name and a (string) type
 *
 * if data_len > 0, will allocate extra memory as part of the
 * attribute block which allows one to do things like pre-allocate the
 * string storage space for a string attribute, or similar. If this is
 * specified, data_ptr must be provided to receive the memory
 * location. The responsibility is transferred to the caller to know
 * not to free this returned memory.
 *
 */
EXR_EXPORT int exr_attr_list_add_by_type(
    exr_file_t *file,
    exr_attribute_list_t *l,
    const char *name,
    const char *type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr );

/** Adds a new attribute to the list with a name and a built-in type
 *
 * if data_len > 0, will allocate extra memory as part of the
 * attribute block which allows one to do things like pre-allocate the
 * string storage space for a string attribute, or similar. If this is
 * specified, data_ptr must be provided to receive the memory
 * location. The responsibility is transferred to the caller to know
 * not to free this returned memory.
 *
 */
EXR_EXPORT int exr_attr_list_add(
    exr_file_t *file,
    exr_attribute_list_t *l,
    const char *name,
    exr_ATTRIBUTE_TYPE_t type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr );

/** Adds a new attribute to the list with a static name (no
 * allocation) and a built-in type
 *
 * if data_len > 0, will allocate extra memory as part of the
 * attribute block which allows one to do things like pre-allocate the
 * string storage space for a string attribute, or similar. If this is
 * specified, data_ptr must be provided to receive the memory
 * location. The responsibility is transferred to the caller to know
 * not to free this returned memory.
 *
 */
EXR_EXPORT int exr_attr_list_add_static_name(
    exr_file_t *file,
    exr_attribute_list_t *l,
    const char *name,
    exr_ATTRIBUTE_TYPE_t type,
    int32_t data_len,
    uint8_t **data_ptr,
    exr_attribute_t **attr );

/** Removes an attribute from the list and frees any associated memory */
EXR_EXPORT int exr_attr_list_remove(
    exr_file_t *file,
    exr_attribute_list_t *l,
    exr_attribute_t *attr );
    
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_H */
