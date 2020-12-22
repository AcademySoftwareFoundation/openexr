# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(IexTest
  main.cpp
  testBaseExc.cpp
)

target_link_libraries(IexTest OpenEXR::Iex)
set_target_properties(IexTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(IexTest PRIVATE OPENEXR_DLL)
endif()
add_test(NAME OpenEXR.Iex COMMAND $<TARGET_FILE:IexTest>)
