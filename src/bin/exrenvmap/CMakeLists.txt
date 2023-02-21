# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable( exrenvmap
  blurImage.cpp
  blurImage.h
  EnvmapImage.cpp
  EnvmapImage.h
  main.cpp
  makeCubeMap.cpp
  makeCubeMap.h
  makeLatLongMap.cpp
  makeLatLongMap.h
  namespaceAlias.h
  readInputImage.cpp
  readInputImage.h
  resizeImage.cpp
  resizeImage.h
)

target_link_libraries(exrenvmap OpenEXR::OpenEXR)
set_target_properties(exrenvmap PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrenvmap DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrenvmap PRIVATE OPENEXR_DLL)
endif()
