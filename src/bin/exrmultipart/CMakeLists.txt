# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmultipart exrmultipart.cpp)
target_link_libraries(exrmultipart OpenEXR::OpenEXR)
set_target_properties(exrmultipart PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
install(TARGETS exrmultipart DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmultipart PRIVATE OPENEXR_DLL)
endif()
