# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

openexr_define_library(OpenEXRCore
  PRIV_EXPORT OPENEXRCORE_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
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
    compress.c
    decompress.c
    pack.c
    unpack.c
    validation.c

    debug.c

  HEADERS
    openexr.h

    openexr_attr.h
    openexr_base.h
    openexr_chunkio.h
    openexr_conf.h
    openexr_context.h
    openexr_decode.h
    openexr_debug.h
    openexr_encode.h
    openexr_part.h
    openexr_std_attr.h
    DEPENDENCIES
    Imath::ImathConfig
    ZLIB::ZLIB
  PRIVATE_DEPS
    ${OPENEXR_EXTRA_MATH_LIB}
  )
