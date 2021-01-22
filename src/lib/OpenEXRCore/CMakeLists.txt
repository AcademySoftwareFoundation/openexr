# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

openexr_define_library(OpenEXRCore
  PRIV_EXPORT OPENEXRCORE_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    openexr_priv_constants.h
    openexr_priv_file.h
    openexr_priv_memory.h
    openexr_priv_structs.h
    openexr_priv_xdr.h

    openexr_attr.c
    openexr_attr_string.c
    openexr_attr_string_vector.c
    openexr_attr_float_vector.c
    openexr_attr_chlist.c
    openexr_attr_opaque.c
    openexr_attr_preview.c

    openexr_req_attr.c
    openexr_std_attr.c
    openexr_file_attr.c

    openexr_base.c
    openexr_file.c
    openexr_memory.c
    openexr_structs.c

    openexr_file_info.c

    openexr_parse_header.c
    openexr_read.c
    openexr_decompress.c

    openexr_validation.c

    openexr_write.c

  HEADERS
    openexr_conf.h
    openexr_base.h

    openexr_attr.h
    openexr_attr_simple.h
    openexr_attr_tiledesc.h
    openexr_attr_string.h
    openexr_attr_string_vector.h
    openexr_attr_float_vector.h
    openexr_attr_chlist.h
    openexr_attr_preview.h
    openexr_attr_opaque.h

    openexr_std_attr.h
    openexr_req_attr.h

    openexr_file.h
    openexr_stream.h

    openexr_file_info.h

    openexr_read.h
    openexr_write.h

    openexr.h

  DEPENDENCIES
    ZLIB::ZLIB
  )

target_link_libraries(OpenEXRCore PUBLIC ${OPENEXR_EXTRA_MATH_LIB})
