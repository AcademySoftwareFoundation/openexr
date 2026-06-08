# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

if (OPENEXR_FORCE_EMBEDDED_CORE)
  message(STATUS "Building Core library as an embedded library")
  set(coreopts EMBEDDED)
endif()

openexr_define_library(OpenEXRCore
  ${coreopts}
  PRIV_EXPORT OPENEXRCORE_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    # old versions of structures
    backward_compatibility.h
    #NB: If you make any of these public, make sure to update the
    # locking macros in the relative source files
    internal_attr.h
    internal_bytes.h
    internal_channel_list.h
    internal_coding.h
    internal_constants.h
    internal_compress.h
    internal_cpuid.h
    internal_decompress.h
    internal_dwa_channeldata.h
    internal_dwa_classifier.h
    internal_dwa_compressor.h
    internal_dwa_decoder.h
    internal_dwa_encoder.h
    internal_dwa_helpers.h
    internal_dwa_simd.h
    internal_file.h
    internal_float_vector.h
    internal_huf.h
    internal_memory.h
    internal_opaque.h
    internal_posix_file_impl.h
    internal_win32_file_impl.h
    internal_preview.h
    internal_string.h
    internal_string_vector.h
    internal_structs.h
    internal_thread.h
    internal_util.h
    internal_xdr.h

    internal_rle.c
    internal_zip.c
    internal_pxr24.c
    internal_b44.c
    internal_b44_table.c
    internal_b44_table_init.c
    internal_piz.c
    internal_ht.cpp
    internal_ht_common.cpp
    internal_dwa.c
    internal_dwa_table.c
    internal_dwa_table_init.c
    internal_huf.c
    internal_zstd.c
    internal_zstd_delta.c
    internal_zstd_shuffle.c

    attributes.c
    string.c
    string_vector.c
    float_vector.c
    channel_list.c
    opaque.c
    bytes.c
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
    compression.c
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
    openexr_compression.h
    openexr_config.h
    openexr_context.h
    openexr_decode.h
    openexr_debug.h
    openexr_encode.h
    openexr_errors.h
    openexr_part.h
    openexr_std_attr.h
    openexr_version.h

  PRIVATE_DEPS
    ${OPENEXR_EXTRA_MATH_LIB}

  DEPENDENCIES
    Imath::Imath
  )

if (DEFINED EXR_DEFLATE_LIB)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(OpenEXRCore PRIVATE ${EXR_DEFLATE_LIB})
  else()
    target_link_libraries(OpenEXRCore PUBLIC ${EXR_DEFLATE_LIB})
  endif()
endif()

if (DEFINED EXR_ZSTD_LIB)

  # External zstd

  if (BUILD_SHARED_LIBS)
    target_link_libraries(OpenEXRCore PRIVATE ${EXR_ZSTD_LIB})
  else()
    target_link_libraries(OpenEXRCore PUBLIC ${EXR_ZSTD_LIB})
  endif()

else()

  # Vendored zstd
  #
  # Compile the vendored zstd sources directly into OpenEXRCore rather than
  # building a separate static library. This bundles the zstd symbols into
  # OpenEXRCore so that a static OpenEXRCore is self-contained: a separate
  # static target is neither installed nor part of the OpenEXR export set,
  # which both breaks install(EXPORT ...) and leaves undefined references for
  # downstream static consumers. OpenEXRCore's hidden visibility keeps the
  # zstd symbols out of the shared library's ABI.

  set(ZSTD_LIB_DIR ${PROJECT_SOURCE_DIR}/external/zstd/lib)

  set(EXR_ZSTD_SOURCES
    ${ZSTD_LIB_DIR}/common/debug.c
    ${ZSTD_LIB_DIR}/common/entropy_common.c
    ${ZSTD_LIB_DIR}/common/error_private.c
    ${ZSTD_LIB_DIR}/common/fse_decompress.c
    ${ZSTD_LIB_DIR}/common/pool.c
    ${ZSTD_LIB_DIR}/common/threading.c
    ${ZSTD_LIB_DIR}/common/xxhash.c
    ${ZSTD_LIB_DIR}/common/zstd_common.c
    ${ZSTD_LIB_DIR}/compress/fse_compress.c
    ${ZSTD_LIB_DIR}/compress/hist.c
    ${ZSTD_LIB_DIR}/compress/huf_compress.c
    ${ZSTD_LIB_DIR}/compress/zstd_compress.c
    ${ZSTD_LIB_DIR}/compress/zstd_compress_literals.c
    ${ZSTD_LIB_DIR}/compress/zstd_compress_sequences.c
    ${ZSTD_LIB_DIR}/compress/zstd_compress_superblock.c
    ${ZSTD_LIB_DIR}/compress/zstd_double_fast.c
    ${ZSTD_LIB_DIR}/compress/zstd_fast.c
    ${ZSTD_LIB_DIR}/compress/zstd_lazy.c
    ${ZSTD_LIB_DIR}/compress/zstd_ldm.c
    ${ZSTD_LIB_DIR}/compress/zstd_opt.c
    ${ZSTD_LIB_DIR}/compress/zstd_preSplit.c
    ${ZSTD_LIB_DIR}/compress/zstdmt_compress.c
    ${ZSTD_LIB_DIR}/decompress/huf_decompress.c
    ${ZSTD_LIB_DIR}/decompress/zstd_ddict.c
    ${ZSTD_LIB_DIR}/decompress/zstd_decompress.c
    ${ZSTD_LIB_DIR}/decompress/zstd_decompress_block.c)

  if (NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|AMD64.*|x86_64.*|X86_64.*")
    enable_language(ASM)
    list(APPEND EXR_ZSTD_SOURCES ${ZSTD_LIB_DIR}/decompress/huf_decompress_amd64.S)
  else()
    set_source_files_properties(${EXR_ZSTD_SOURCES} PROPERTIES
      COMPILE_DEFINITIONS ZSTD_DISABLE_ASM)
  endif()

  target_sources(OpenEXRCore PRIVATE ${EXR_ZSTD_SOURCES})
  target_include_directories(OpenEXRCore PRIVATE ${ZSTD_LIB_DIR})

endif()

if (DEFINED EXR_OPENJPH_LIB)

  # External OpenJPH

  if (BUILD_SHARED_LIBS)
    target_link_libraries(OpenEXRCore PRIVATE ${EXR_OPENJPH_LIB})
  else()
    target_link_libraries(OpenEXRCore PUBLIC ${EXR_OPENJPH_LIB})
  endif()

  if (openjph_VERSION AND openjph_VERSION VERSION_LESS "0.23")
    # OpenJPH 0.22 and before incorrectly appends "openjph" to INTERFACE_INCLUDE_DIRECTORIES
    # so OpenEXR's "#include <openjph/ojph_arch.h>" does not work.
    # Strip the "openjph" from the setting in this case. This allows the
    # #include statements in OpenEXRCore/internal_ht.cpp  to work properly for all openjph versions.
    get_target_property(OPENJPH_INCLUDE_DIR openjph INTERFACE_INCLUDE_DIRECTORIES)
    if (NOT OPENJPH_INCLUDE_DIR)
      message(FATAL_ERROR "failed to set openjph header directory, version ${openjph_VERSION}")
    endif()
    string(REGEX REPLACE "/openjph/?$" "" OPENJPH_PARENT_INCLUDE_DIR "${OPENJPH_INCLUDE_DIR}")
    set_target_properties(openjph PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENJPH_PARENT_INCLUDE_DIR}"
    )
    unset(OPENJPH_INCLUDE_DIR)
    unset(OPENJPH_PARENT_INCLUDE_DIR)
  endif()

else()

  # Vendored OpenJPH
  # Note that the vendored OpenJPH cmake config forces a static build

  set(OJPH_BUILD_EXECUTABLES OFF CACHE BOOL "Disable OpenJPH command-line tools" FORCE)
  set(OPENJPH_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/OpenJPH)
  set(OPENJPH_BINARY_DIR ${PROJECT_BINARY_DIR}/external/OpenJPH)
  add_subdirectory(${OPENJPH_SOURCE_DIR}
                   ${OPENJPH_BINARY_DIR}
                   EXCLUDE_FROM_ALL)
  set_target_properties(openjph PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
  )
  target_link_libraries(OpenEXRCore PRIVATE openjph)
  target_include_directories(OpenEXRCore PRIVATE ${OPENJPH_SOURCE_DIR}/src/core)

endif()

