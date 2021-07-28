# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(OpenEXRCoreTest
  base_units.cpp
  compression.cpp
  general_attr.cpp
  main.cpp
  read.cpp
  write.cpp
  deep.cpp
  )
target_compile_definitions(OpenEXRCoreTest PRIVATE ILM_IMF_TEST_IMAGEDIR="${CMAKE_CURRENT_SOURCE_DIR}/../OpenEXRTest/")
# TODO: remove exr once we are happy everything is identical
#target_link_libraries(OpenEXRCoreTest OpenEXR::OpenEXRCore)
target_link_libraries(OpenEXRCoreTest OpenEXR::OpenEXRCore OpenEXR::OpenEXR)
target_compile_definitions(OpenEXRCoreTest PRIVATE
  COMP_MAJ=${OpenEXR_VERSION_MAJOR}
  COMP_MIN=${OpenEXR_VERSION_MINOR}
  COMP_PATCH=${OpenEXR_VERSION_PATCH}
  COMP_EXTRA="\\"${OPENEXR_VERSION_RELEASE_TYPE}\\""
  )
set_target_properties(OpenEXRCoreTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

target_link_libraries(OpenEXRCoreTest Imath::Imath)

if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(OpenEXRCoreTest PRIVATE OPENEXR_DLL)
endif()

add_executable(CorePerfTest
  performance.cpp)
target_link_libraries(CorePerfTest OpenEXR::OpenEXRCore OpenEXR::OpenEXR)
set_target_properties(CorePerfTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(CorePerfTest PRIVATE OPENEXR_DLL)
endif()

function(DEFINE_OPENEXRCORE_TESTS)
  foreach(curtest IN LISTS ARGN)
    # CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
    add_test(NAME OpenEXRCore.${curtest} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:OpenEXRCoreTest> ${curtest})
  endforeach()
endfunction()

define_openexrcore_tests(
 testBase
 testBaseErrors
 testBaseLimits
 testBaseDebug
 testXDR

 testAttrSizes
 testAttrStrings
 testAttrStringVectors
 testAttrFloatVectors
 testAttrChlists
 testAttrPreview
 testAttrOpaque
 testAttrHandler
 testAttrLists

 testReadBadArgs
 testReadBadFiles
 testOpenScans
 testOpenTiles
 testOpenMultiPart
 testOpenDeep
 testReadScans
 testReadTiles
 testReadMultiPart
 testReadDeep
 testReadUnpack

 testWriteBadArgs
 testWriteBadFiles
 testUpdateMeta
 testWriteBaseHeader
 testStartWriteScan
 testStartWriteDeepScan
 testStartWriteTile
 testStartWriteDeepTile
 testWriteAttrs
 testWriteScans
 testWriteTiles
 testWriteMultiPart
 testWriteDeep

 testHUF
 testNoCompression
 testRLECompression
 testZIPCompression
 testZIPSCompression
 testPIZCompression
 testPXR24Compression
 testB44Compression
 testB44ACompression
 testDWAACompression
 testDWABCompression
 testDeepNoCompression
 testDeepZIPCompression
 testDeepZIPSCompression
)
