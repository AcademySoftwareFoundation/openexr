# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(OpenEXRUtilTest
  main.cpp
  testFlatImage.cpp
  testFlatImage.h
  testDeepImage.cpp
  testDeepImage.h
  testIO.cpp
  testIO.h
 )
target_include_directories(OpenEXRUtilTest PRIVATE ../OpenEXRTest)
target_link_libraries(OpenEXRUtilTest OpenEXR::OpenEXRUtil)
set_target_properties(OpenEXRUtilTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(OpenEXRUtilTest PRIVATE OPENEXR_DLL)
endif()

function(DEFINE_OPENEXR_UTIL_TESTS)
  foreach(curtest IN LISTS ARGN)
    # CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
    add_test(NAME OpenEXRUtil.${curtest} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:OpenEXRUtilTest> ${curtest})
  endforeach()
endfunction()

define_openexr_util_tests(
  testFlatImage
  testDeepImage
  testIO
)
