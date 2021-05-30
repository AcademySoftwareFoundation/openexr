# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(OpenEXRCoreTest
  main.cpp
  base_units.cpp
  general_attr.cpp
  read.cpp
  write.cpp
  )
target_compile_definitions(OpenEXRCoreTest PRIVATE ILM_IMF_TEST_IMAGEDIR="${CMAKE_CURRENT_SOURCE_DIR}/../OpenEXRTest/")
target_link_libraries(OpenEXRCoreTest OpenEXR::OpenEXRCore)
target_compile_definitions(OpenEXRCoreTest PRIVATE
  COMP_MAJ=${OpenEXR_VERSION_MAJOR}
  COMP_MIN=${OpenEXR_VERSION_MINOR}
  COMP_PATCH=${OpenEXR_VERSION_PATCH}
  COMP_EXTRA="\\"${OPENEXR_VERSION_RELEASE_TYPE}\\""
  )
set_target_properties(OpenEXRCoreTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

if(WIN32)
  target_link_libraries(OpenEXRCoreTest Imath::Imath)
endif()

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

#add_test(NAME OpenEXR.Core COMMAND $<TARGET_FILE:OpenEXRCoreTest>)
function(DEFINE_OPENEXRCORE_TESTS)
  foreach(curtest IN LISTS ARGN)
    add_test(NAME OpenEXRCore.${curtest} COMMAND $<TARGET_FILE:OpenEXRCoreTest> ${curtest})
  endforeach()
endfunction()

define_openexrcore_tests(
 testBase
 testBaseErrors
 testBaseLimits
 testBaseDebug

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
)
