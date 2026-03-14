
file(GLOB CODESTREAM       "codestream/*.cpp" "codestream/*.h")
file(GLOB CODESTREAM_SSE   "codestream/*_sse.cpp")
file(GLOB CODESTREAM_SSE2  "codestream/*_sse2.cpp")
file(GLOB CODESTREAM_AVX   "codestream/*_avx.cpp")
file(GLOB CODESTREAM_AVX2  "codestream/*_avx2.cpp")
file(GLOB CODESTREAM_WASM  "codestream/*_wasm.cpp")
file(GLOB CODING           "coding/*.cpp" "coding/*.h")
file(GLOB CODING_SSSE3     "coding/*_ssse3.cpp")
file(GLOB CODING_WASM      "coding/*_wasm.cpp")
file(GLOB CODING_AVX2      "coding/*_avx2.cpp")
file(GLOB CODING_AVX512    "coding/*_avx512.cpp")
file(GLOB COMMON           "openjph/*.h")
file(GLOB OTHERS           "others/*.cpp" "others/*.c")
file(GLOB TRANSFORM        "transform/*.cpp" "transform/*.h")
file(GLOB TRANSFORM_SSE    "transform/*_sse.cpp")
file(GLOB TRANSFORM_SSE2   "transform/*_sse2.cpp")
file(GLOB TRANSFORM_AVX    "transform/*_avx.cpp")
file(GLOB TRANSFORM_AVX2   "transform/*_avx2.cpp")
file(GLOB TRANSFORM_AVX512 "transform/*_avx512.cpp")
file(GLOB TRANSFORM_WASM   "transform/*_wasm.cpp")

list(REMOVE_ITEM CODESTREAM ${CODESTREAM_SSE} ${CODESTREAM_SSE2} ${CODESTREAM_AVX} ${CODESTREAM_AVX2} ${CODESTREAM_WASM})
list(REMOVE_ITEM CODING ${CODING_SSSE3} ${CODING_WASM} ${CODING_AVX2} ${CODING_AVX512})
list(REMOVE_ITEM TRANSFORM ${TRANSFORM_SSE} ${TRANSFORM_SSE2} ${TRANSFORM_AVX} ${TRANSFORM_AVX2} ${TRANSFORM_AVX512} ${TRANSFORM_WASM})
list(APPEND SOURCES ${CODESTREAM} ${CODING} ${COMMON} ${OTHERS} ${TRANSFORM})

source_group("codestream"        FILES ${CODESTREAM})
source_group("coding"            FILES ${CODING})
source_group("common"            FILES ${COMMON})
source_group("others"            FILES ${OTHERS})
source_group("transform"         FILES ${TRANSFORM})

if(EMSCRIPTEN)
  if (OJPH_ENABLE_WASM_SIMD)
    list(APPEND SOURCES ${CODESTREAM_WASM} ${CODING_WASM} ${TRANSFORM_WASM})
    source_group("codestream" FILES ${CODESTREAM_WASM})
    source_group("coding" FILES ${CODING_WASM})
    source_group("transform" FILES ${TRANSFORM_WASM})
  endif()
else()
  if (NOT OJPH_DISABLE_SIMD)
    if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_X86_64")
      OR ("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_I386")
      OR MULTI_GEN_X86_64)

      if (NOT OJPH_DISABLE_SSE)
        list(APPEND SOURCES ${CODESTREAM_SSE} ${TRANSFORM_SSE})
        source_group("codestream" FILES ${CODESTREAM_SSE})
        source_group("transform" FILES ${TRANSFORM_SSE})
      endif()
      if (NOT OJPH_DISABLE_SSE2)
        list(APPEND SOURCES ${CODESTREAM_SSE2} ${TRANSFORM_SSE2})
        source_group("codestream" FILES ${CODESTREAM_SSE2})
        source_group("transform" FILES ${TRANSFORM_SSE2})
      endif()
      if (NOT OJPH_DISABLE_SSSE3)
        list(APPEND SOURCES ${CODING_SSSE3})
        source_group("coding" FILES ${CODING_SSSE3})
      endif()
      if (NOT OJPH_DISABLE_AVX)
        list(APPEND SOURCES ${CODESTREAM_AVX} ${TRANSFORM_AVX})
        source_group("codestream" FILES ${CODESTREAM_AVX})
        source_group("transform" FILES ${TRANSFORM_AVX})
      endif()
      if (NOT OJPH_DISABLE_AVX2)
        list(APPEND SOURCES ${CODESTREAM_AVX2} ${TRANSFORM_AVX2} ${CODING_AVX2})
        source_group("codestream" FILES ${CODESTREAM_AVX2})
        source_group("transform" FILES ${TRANSFORM_AVX2})
        source_group("coding" FILES ${CODING_AVX2})
      endif()
      if (NOT OJPH_DISABLE_AVX512)
        list(APPEND SOURCES ${CODING_AVX512} ${TRANSFORM_AVX512})
        source_group("coding" FILES ${CODING_AVX512})
        source_group("transform" FILES ${TRANSFORM_AVX512})
      endif()

      # Set compilation flags
      if (MSVC)
        set_source_files_properties(codestream/ojph_codestream_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
        set_source_files_properties(codestream/ojph_codestream_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        set_source_files_properties(coding/ojph_block_decoder_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        set_source_files_properties(coding/ojph_block_encoder_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        set_source_files_properties(coding/ojph_block_encoder_avx512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX512")
        set_source_files_properties(transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
        set_source_files_properties(transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        set_source_files_properties(transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX")
        set_source_files_properties(transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
        set_source_files_properties(transform/ojph_transform_avx512.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX512")
      else()
        set_source_files_properties(codestream/ojph_codestream_sse.cpp PROPERTIES COMPILE_FLAGS -msse)
        set_source_files_properties(codestream/ojph_codestream_sse2.cpp PROPERTIES COMPILE_FLAGS -msse2)
        set_source_files_properties(codestream/ojph_codestream_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
        set_source_files_properties(codestream/ojph_codestream_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
        set_source_files_properties(coding/ojph_block_decoder_ssse3.cpp PROPERTIES COMPILE_FLAGS -mssse3)
        set_source_files_properties(coding/ojph_block_decoder_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
        set_source_files_properties(coding/ojph_block_encoder_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
        set_source_files_properties(coding/ojph_block_encoder_avx512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512cd")
        set_source_files_properties(transform/ojph_colour_sse.cpp PROPERTIES COMPILE_FLAGS -msse)
        set_source_files_properties(transform/ojph_colour_sse2.cpp PROPERTIES COMPILE_FLAGS -msse2)
        set_source_files_properties(transform/ojph_colour_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
        set_source_files_properties(transform/ojph_colour_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
        set_source_files_properties(transform/ojph_transform_sse.cpp PROPERTIES COMPILE_FLAGS -msse)
        set_source_files_properties(transform/ojph_transform_sse2.cpp PROPERTIES COMPILE_FLAGS -msse2)
        set_source_files_properties(transform/ojph_transform_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
        set_source_files_properties(transform/ojph_transform_avx2.cpp PROPERTIES COMPILE_FLAGS -mavx2)
        set_source_files_properties(transform/ojph_transform_avx512.cpp PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512cd")
      endif()
    endif()

    if (("${OJPH_TARGET_ARCH}" MATCHES "OJPH_ARCH_ARM") OR MULTI_GEN_ARM64)

    endif()

  endif()

endif()

## Set debug postfix for different platforms
if (MSVC)
  if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX "d")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX "_d")
  endif()
endif()

add_library(openjph STATIC ${SOURCES})

## The option BUILD_SHARED_LIBS
if (BUILD_SHARED_LIBS AND WIN32)
  target_compile_definitions(openjph PRIVATE OJPH_BUILD_SHARED_LIBRARY)
endif()

## include library version/name
set_target_properties(openjph PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(openjph PUBLIC _FILE_OFFSET_BITS=64)
target_include_directories(openjph PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/openjph> $<INSTALL_INTERFACE:include>)

## This is to check if aligned_alloc or posix_memalign is available
# We want the code to compile for C11 and C++11.
# std::aligned_alloc is only availabe in C++17.
# So here we try to see which API is available and adapt the code to use it
if (NOT MSVC)
  include(CheckSymbolExists)
  check_symbol_exists(aligned_alloc "stdlib.h" OJPH_ALIGNED_ALLOC_EXISTS)
  if (OJPH_ALIGNED_ALLOC_EXISTS)
    target_compile_definitions(openjph PRIVATE OJPH_ALIGNED_ALLOC_EXISTS)
  else()
    check_symbol_exists(posix_memalign "stdlib.h" OJPH_POSIX_MEMALIGN_EXISTS)
    if (OJPH_POSIX_MEMALIGN_EXISTS)
      target_compile_definitions(openjph PRIVATE OJPH_POSIX_MEMALIGN_EXISTS)
    endif()
  endif()
endif()

if (MSVC)
  set(OJPH_LIB_NAME_STRING "openjph.${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}")
  set_target_properties(openjph
    PROPERTIES
      OUTPUT_NAME "openjph.${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}")
else()
  set(OJPH_LIB_NAME_STRING "openjph.${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}")
  set_target_properties(openjph
    PROPERTIES
      SOVERSION "${OPENJPH_VERSION_MAJOR}.${OPENJPH_VERSION_MINOR}"
      VERSION "${OPENJPH_VERSION}")
endif()

install(TARGETS openjph
  EXPORT openjph-targets
)

install(DIRECTORY openjph/
  DESTINATION include/openjph
  FILES_MATCHING
  PATTERN "*.h"
)
