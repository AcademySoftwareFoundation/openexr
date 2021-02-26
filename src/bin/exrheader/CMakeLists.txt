# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrheader main.cpp)
target_link_libraries(exrheader OpenEXR::OpenEXR)
set_target_properties(exrheader PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
install(TARGETS exrheader DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrheader PRIVATE OPENEXR_DLL)
endif()
