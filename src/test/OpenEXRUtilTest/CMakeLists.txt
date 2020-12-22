# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(OpenEXRUtilTest
  main.cpp
  testFlatImage.cpp
  testDeepImage.cpp
  testIO.cpp
 )
target_link_libraries(OpenEXRUtilTest OpenEXR::OpenEXRUtil)
set_target_properties(OpenEXRUtilTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(OpenEXRUtilTest PRIVATE OPENEXR_DLL)
endif()

function(DEFINE_OPENEXR_UTIL_TESTS)
  foreach(curtest IN LISTS ARGN)
    add_test(NAME OpenEXRUtil.${curtest} COMMAND $<TARGET_FILE:OpenEXRUtilTest> ${curtest})
  endforeach()
endfunction()
#add_test(NAME OpenEXR.OpenEXRUtil COMMAND $<TARGET_FILE:OpenEXRUtilTest>)

define_openexr_util_tests(
  testFlatImage
  testDeepImage
  testIO
)
