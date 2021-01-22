/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_OPAQUE_H
#define OPENEXR_ATTR_OPAQUE_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include "openexr_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup custom attribute storage and functions
 * @brief These are a group of functions for handling custom / unspecified attribute types
 * @{
 */

/** Custom storage structure */
typedef struct
{
    int32_t size;
	int32_t unpacked_size;

	size_t alloc_size; /**< if this is non-zero, the struct owns the data, if 0, is a const ref */
    void *packed_data;
	/** when an application wants to have custom data, they can store an unpacked form here which will
	 * be freed upon destruction of the attribute */
	void *unpacked_data;

	/* an application can register an attribute handler which then
	 * fills in these function pointers. This allows a user to delay
	 * the expansion of the custom type until access is desired, and
	 * similarly, to delay the packing of the data until write time */
    int (*unpack_func_ptr)( const void *data, int32_t attrsize, int32_t *outsize, void **outbuffer );
    int (*pack_func_ptr)( const void *data, int32_t datasize, int32_t *outsize, void **outbuffer );
    void (*destroy_func_ptr)( void *data, int32_t attrsize );
} exr_attr_opaquedata_t;

EXR_EXPORT int exr_attr_opaquedata_init(
    exr_file_t *, exr_attr_opaquedata_t *, size_t );
EXR_EXPORT int exr_attr_opaquedata_create(
    exr_file_t *, exr_attr_opaquedata_t *, size_t, const void * );
EXR_EXPORT void exr_attr_opaquedata_destroy( exr_attr_opaquedata_t *ud );

/** If an unpack routine was registered, this unpacks the opaque data, returning the pointer and size.
 *
 * The unpacked pointer is stored internally and will be freed during destroy */
EXR_EXPORT void *exr_attr_opaquedata_unpack(
    exr_file_t *, exr_attr_opaquedata_t *, int32_t *sz );
/** If a pack routine was registered, this packs the opaque data, returning the pointer and size.
 *
 * The packed pointer is stored internally and will be freed during destroy */
EXR_EXPORT void *exr_attr_opaquedata_pack(
    exr_file_t *, exr_attr_opaquedata_t *, int32_t *sz );
/** Assigns unpacked data
 *
 * Assuming the appropriate handlers have been registered, assigns the
 * unpacked data to the provided value. This memory will be freed at
 * destruction time
 */
EXR_EXPORT int exr_attr_opaquedata_set_unpacked(
    exr_file_t *, exr_attr_opaquedata_t *, void *unpacked, int32_t sz );

/** Any opaque data entry of the specified type is tagged with these
 * functions enabling downstream users to unpack (or pack) the data.
 */
EXR_EXPORT int exr_register_attr_handler(
    exr_file_t *file, const char *type,
    int (*unpack_func_ptr)( const void *data, int32_t attrsize, int32_t *outsize, void **outbuffer ),
    int (*pack_func_ptr)( const void *data, int32_t datasize, int32_t *outsize, void **outbuffer ),
    void (*destroy_func_ptr)( void *data, int32_t datasize ) );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_OPAQUE_H */
