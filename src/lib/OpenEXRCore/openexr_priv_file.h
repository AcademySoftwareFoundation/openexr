/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_FILE_UTIL_H
#define OPENEXR_PRIVATE_FILE_UTIL_H

#include "openexr_priv_structs.h"

#define EXR_FILE_VERSION 2
#define EXR_FILE_VERSION_MASK 0x000000FF
#define EXR_TILED_FLAG 0x00000200
#define EXR_LONG_NAMES_FLAG 0x00000400
#define EXR_NON_IMAGE_FLAG 0x00000800
#define EXR_MULTI_PART_FLAG 0x00001000
#define EXR_VALID_FLAGS (EXR_TILED_FLAG | EXR_LONG_NAMES_FLAG | EXR_NON_IMAGE_FLAG | EXR_MULTI_PART_FLAG)

/* in openexr_parse_header.c, reads the header and populates the file structure */
int priv_parse_header( EXR_TYPE(PRIV_FILE) *file );

/* in openexr_validate.c, functions to validate the header during read / pre-write */
int priv_validate_read_part( EXR_TYPE(PRIV_FILE) *file, EXR_TYPE(PRIV_PART) *curpart );
int priv_validate_write_part( EXR_TYPE(PRIV_FILE) *file, EXR_TYPE(PRIV_PART) *curpart );

#endif /* OPENEXR_PRIVATE_FILE_UTIL_H */
