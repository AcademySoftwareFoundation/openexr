# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmakepreview
  main.cpp
  makePreview.cpp
)
target_link_libraries(exrmakepreview OpenEXR::IlmImf)
set_target_properties(exrmakepreview PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
install(TARGETS exrmakepreview DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(exrmakepreview PRIVATE OPENEXR_DLL)
endif()
