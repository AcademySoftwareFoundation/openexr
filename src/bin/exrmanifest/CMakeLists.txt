# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(exrmanifest main.cpp)
target_link_libraries(exrmanifest OpenEXR::OpenEXR)
set_target_properties(exrmanifest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrmanifest DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmanifest PRIVATE OPENEXR_DLL)
endif()
