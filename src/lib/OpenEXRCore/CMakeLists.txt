# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

openexr_define_library(OpenEXRCore
  PRIV_EXPORT OPENEXRCORE_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    # old versions of structures
    backward_compatibility.h
    #NB: If you make any of these public, make sure to update the
    # locking macros in the relative source files
    internal_attr.h
    internal_channel_list.h
    internal_coding.h
    internal_constants.h
    internal_compress.h
    internal_decompress.h
    internal_file.h
    internal_float_vector.h
    internal_memory.h
    internal_opaque.h
    internal_posix_file_impl.h
    internal_win32_file_impl.h
    internal_preview.h
    internal_string.h
    internal_string_vector.h
    internal_structs.h
    internal_xdr.h

    internal_rle.c
    internal_zip.c
    internal_pxr24.c
    internal_b44.c
    internal_b44_table.c
    internal_piz.c
    internal_dwa.c
    internal_huf.c

    attributes.c
    string.c
    string_vector.c
    float_vector.c
    channel_list.c
    opaque.c
    preview.c

    base.c
    context.c
    memory.c
    internal_structs.c

    part.c
    part_attr.c
    std_attr.c

    parse_header.c
    write_header.c

    chunk.c
    coding.c
    decoding.c
    encoding.c
    pack.c
    unpack.c
    validation.c

    debug.c

  HEADERS
    openexr.h

    openexr_attr.h
    openexr_base.h
    openexr_chunkio.h
    openexr_coding.h
    openexr_conf.h
    openexr_context.h
    openexr_decode.h
    openexr_debug.h
    openexr_encode.h
    openexr_errors.h
    openexr_part.h
    openexr_std_attr.h
  DEPENDENCIES
    ZLIB::ZLIB
  PRIVATE_DEPS
    ${OPENEXR_EXTRA_MATH_LIB}
  )

# when building with an internal imath, this isn't generated until
# install time, so need to use private header only include path (we
# aren't linking to imath or anything c++)
if (TARGET Imath::ImathConfig)
  target_link_libraries(OpenEXRCore PRIVATE Imath::ImathConfig)
else()
  target_include_directories(OpenEXRCore PRIVATE ${IMATH_HEADER_ONLY_INCLUDE_DIRS})
endif()
