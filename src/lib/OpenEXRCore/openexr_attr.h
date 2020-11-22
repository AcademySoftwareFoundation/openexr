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
    EXR_DEF(ATTR_UNKNOWN) = 0, /**< type indicating an error or uninitialized attribute */
    EXR_DEF(ATTR_BOX2I), /**< integer region definition. @see exr_box2i */
    EXR_DEF(ATTR_BOX2F), /**< float region definition. @see exr_box2f */
    EXR_DEF(ATTR_CHLIST), /**< Definition of channels in file @see exr_chlist_entry */
    EXR_DEF(ATTR_CHROMATICITIES), /**< Values to specify color space of colors in file @see exr_chromaticities */
    EXR_DEF(ATTR_COMPRESSION), /**< uint8_t declaring compression present */
    EXR_DEF(ATTR_DOUBLE), /**< double precision floating point number */
    EXR_DEF(ATTR_ENVMAP), /**< uint8_t declaring environment map type */
    EXR_DEF(ATTR_FLOAT), /**< a normal (4 byte) precision floating point number */
    EXR_DEF(ATTR_FLOAT_VECTOR), /**< a list of normal (4 byte) precision floating point numbers */
    EXR_DEF(ATTR_INT), /**< a 32-bit signed integer value */
    EXR_DEF(ATTR_KEYCODE), /**< structure recording keycode @see exr_keycode */
    EXR_DEF(ATTR_LINEORDER), /**< uint8_t declaring scanline ordering */
    EXR_DEF(ATTR_M33F), /**< 9 32-bit floats representing a 3x3 matrix */
    EXR_DEF(ATTR_M33D), /**< 9 64-bit floats representing a 3x3 matrix */
    EXR_DEF(ATTR_M44F), /**< 16 32-bit floats representing a 4x4 matrix */
    EXR_DEF(ATTR_M44D), /**< 16 64-bit floats representing a 4x4 matrix */
    EXR_DEF(ATTR_PREVIEW), /**< 2 unsigned ints followed by 4 x w x h uint8_t image */
    EXR_DEF(ATTR_RATIONAL), /**< int followed by unsigned int */
    EXR_DEF(ATTR_STRING), /**< int (length) followed by char string data */
    EXR_DEF(ATTR_STRING_VECTOR), /**< 0 or more text strings (int + string). number is based on attribute size */
    EXR_DEF(ATTR_TILEDESC), /**< 2 unsigned ints xSize, ySize followed by mode */
    EXR_DEF(ATTR_TIMECODE), /**< 2 unsigned ints time and flags, user data */
    EXR_DEF(ATTR_V2I), /**< pair of 32-bit integers */
    EXR_DEF(ATTR_V2F), /**< pair of 32-bit floats */
    EXR_DEF(ATTR_V2D), /**< pair of 64-bit floats */
    EXR_DEF(ATTR_V3I), /**< set of 3 32-bit integers */
    EXR_DEF(ATTR_V3F), /**< set of 3 32-bit floats */
    EXR_DEF(ATTR_V3D), /**< set of 3 64-bit floats */
    EXR_DEF(ATTR_OPAQUE), /**< user / unknown provided type */
    EXR_DEF(ATTR_LAST_KNOWN_TYPE)
} EXR_TYPE(ATTRIBUTE_TYPE);

/** storage, name and type information for an attribute */
typedef struct
{
	const char *name; /**< name of the attribute */
	const char *type_name; /**< string type name of the attribute */
	uint8_t name_length; /**< length of name string (short flag is 31 max, long allows 255) */
	uint8_t type_name_length; /**< length of type string (short flag is 31 max, long allows 255) */
    EXR_TYPE(ATTRIBUTE_TYPE) type; /**< enum of the attribute type */

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

        EXR_TYPE(attr_box2i) *box2i;
        EXR_TYPE(attr_box2f) *box2f;
        EXR_TYPE(attr_chlist) *chlist;
        EXR_TYPE(attr_chromaticities) *chromaticities;
        EXR_TYPE(attr_keycode) *keycode;
        EXR_TYPE(attr_float_vector) *floatvector;
        EXR_TYPE(attr_m33f) *m33f;
        EXR_TYPE(attr_m33d) *m33d;
        EXR_TYPE(attr_m44f) *m44f;
        EXR_TYPE(attr_m44d) *m44d;
        EXR_TYPE(attr_preview) *preview;
        EXR_TYPE(attr_rational) *rational;
        EXR_TYPE(attr_string) *string;
        EXR_TYPE(attr_string_vector) *stringvector;
        EXR_TYPE(attr_tiledesc) *tiledesc;
        EXR_TYPE(attr_timecode) *timecode;
        EXR_TYPE(attr_v2i) *v2i;
        EXR_TYPE(attr_v2f) *v2f;
        EXR_TYPE(attr_v2d) *v2d;
        EXR_TYPE(attr_v3i) *v3i;
        EXR_TYPE(attr_v3f) *v3f;
        EXR_TYPE(attr_v3d) *v3d;
        EXR_TYPE(attr_opaquedata) *opaque;
        uint8_t *rawptr;
    };
} EXR_TYPE(attribute);

typedef struct
{
    int num_attributes; /**< number of attribute entries in the list */
    int num_alloced; /**< allocation count. if > 0, attribute list owns pointer */
    EXR_TYPE(attribute) **entries; /**< creation order list of attributes */
    EXR_TYPE(attribute) **sorted_entries; /**< sorted order list of attributes for fast lookup */
} EXR_TYPE(attribute_list);

/** Frees memory for all the owned attributes in the list as well as the list itself */
EXR_EXPORT void EXR_FUN(attr_list_destroy)(
    EXR_TYPE(attribute_list) *l );

/** Computes the number of bytes required to store this attribute list in a file */
EXR_EXPORT uint64_t EXR_FUN(attr_list_compute_size)(
    EXR_TYPE(attribute_list) *l );

/** Finds an attribute in the list by name */
EXR_EXPORT EXR_TYPE(attribute) *EXR_FUN(attr_list_find_by_name)(
    EXR_TYPE(FILE) *file,
    EXR_TYPE(attribute_list) *l,
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
EXR_EXPORT int EXR_FUN(attr_list_add_by_type)(
    EXR_TYPE(FILE) *file,
    EXR_TYPE(attribute_list) *l,
    const char *name,
    const char *type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr );

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
EXR_EXPORT int EXR_FUN(attr_list_add)(
    EXR_TYPE(FILE) *file,
    EXR_TYPE(attribute_list) *l,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr );

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
EXR_EXPORT int EXR_FUN(attr_list_add_static_name)(
    EXR_TYPE(FILE) *file,
    EXR_TYPE(attribute_list) *l,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    int32_t data_len,
    uint8_t **data_ptr,
    EXR_TYPE(attribute) **attr );

/** Removes an attribute from the list and frees any associated memory */
EXR_EXPORT int EXR_FUN(attr_list_remove)(
    EXR_TYPE(FILE) *file,
    EXR_TYPE(attribute_list) *l,
    EXR_TYPE(attribute) *attr );
    
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_H */
