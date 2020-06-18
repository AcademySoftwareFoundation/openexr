# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)
target_link_libraries(exrmultiview OpenEXR::IlmImf)
set_target_properties(exrmultiview PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
install(TARGETS exrmultiview DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(exrmultiview PRIVATE OPENEXR_DLL)
endif()
