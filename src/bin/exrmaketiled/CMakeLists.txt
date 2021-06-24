# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors (c) to the OpenEXR Project.

add_executable(exrmaketiled 
  Image.cpp
  main.cpp
  makeTiled.cpp
)
target_link_libraries(exrmaketiled OpenEXR::OpenEXR)
set_target_properties(exrmaketiled PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrmaketiled DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmaketiled PRIVATE OPENEXR_DLL)
endif()
