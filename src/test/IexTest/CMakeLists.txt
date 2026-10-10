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
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(IexTest PRIVATE OPENEXR_DLL)
endif()
target_compile_features(IexTest
  PRIVATE cxx_std_${OPENEXR_CXX_STANDARD}
)

# CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
add_test(NAME OpenEXR.Iex COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:IexTest>)

# "portability" tests: an abbreviated subset of the test suite run by
# specialty CI jobs (alternate OS/build-variant jobs) to test the
# behaviors most likely to differ across OS/toolchain/libc.
# Primary Linux/macOS/Windows jobs run the complete test suite.
set_tests_properties(OpenEXR.Iex PROPERTIES LABELS portability)
