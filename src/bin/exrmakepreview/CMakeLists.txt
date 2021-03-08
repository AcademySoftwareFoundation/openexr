# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(exrmakepreview
  main.cpp
  makePreview.cpp
)
target_link_libraries(exrmakepreview OpenEXR::OpenEXR)
set_target_properties(exrmakepreview PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrmakepreview DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmakepreview PRIVATE OPENEXR_DLL)
endif()
