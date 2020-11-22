/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_CORE_FILE_ATTR_H
#define OPENEXR_CORE_FILE_ATTR_H

#include "openexr_attr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup File attribute query and creation utililities
 * @brief These are a group of functions for for interacting with attributes in a file.
 * @{
 */

/** @brief Retrieves a count of how many attributes exist in that part
 * of the file.
 *
 *  @return the count of attributes. Upon error, the return value will be
 *  negative, and be the negative of the error code
 */
EXR_EXPORT int EXR_FUN(attr_count)( EXR_TYPE(FILE) *file, int part_index );

/** @brief Attempts to find the specified attribute by name
 *
 * @return returns a pointer to the attribute, or NULL if it is not
 * found or there is an error
 */
EXR_EXPORT EXR_TYPE(attribute) *EXR_FUN(attr_find_by_name)(
    EXR_TYPE(FILE) *file, int part_index, const char *name );

/** Returns the attribute list for the specified part.
 *
 * It is expected that the application observe correctness, and if a
 * file has been opened for read, it will only read the values here.
 */
EXR_EXPORT EXR_TYPE(attribute_list) *EXR_FUN(get_attribute_list)(
    EXR_TYPE(FILE) *file, int part_index );

/** Declare an attribute within the file.
 *
 * Only valid when a file is opened for write.
 */
EXR_EXPORT int EXR_FUN(attr_declare_by_type)(
    EXR_TYPE(FILE) *file,
    int part_index,
    const char *name,
    const char *type,
    EXR_TYPE(attribute) **newattr );

/** @brief Declare an attribute within the file. */
EXR_EXPORT int EXR_FUN(attr_declare)(
    EXR_TYPE(FILE) *file,
    int part_index,
    const char *name,
    EXR_TYPE(ATTRIBUTE_TYPE) type,
    EXR_TYPE(attribute) **newattr );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_CORE_FILE_ATTR_H */
