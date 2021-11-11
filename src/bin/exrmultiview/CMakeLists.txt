# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(exrmultiview
  Image.cpp
  main.cpp
  makeMultiView.cpp
)
target_link_libraries(exrmultiview OpenEXR::OpenEXR)
set_target_properties(exrmultiview PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrmultiview DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmultiview PRIVATE OPENEXR_DLL)
endif()
