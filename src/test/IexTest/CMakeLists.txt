# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(IexTest
  main.cpp
  testBaseExc.cpp
  testBaseExc.h
  testMathExc.cpp
  testMathExc.h
  mathFuncs.cpp
  mathFuncs.h
)

target_link_libraries(IexTest OpenEXR::Iex)
set_target_properties(IexTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(IexTest PRIVATE OPENEXR_DLL)
endif()

# CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
add_test(NAME OpenEXR.Iex COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:IexTest>)
