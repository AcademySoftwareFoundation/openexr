# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrcheck main.cpp)
target_link_libraries(exrcheck OpenEXR::OpenEXR OpenEXR::OpenEXRUtil)
set_target_properties(exrcheck PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
# install(TARGETS exrcheck DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrcheck PRIVATE OPENEXR_DLL)
endif()
