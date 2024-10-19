# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrconv main.cpp)
target_link_libraries(exrconv OpenEXR::OpenEXR)
set_target_properties(exrconv PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrconv DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(exrconv PRIVATE OPENEXR_DLL)
endif()
