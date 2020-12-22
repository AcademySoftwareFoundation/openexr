# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable( exrenvmap
  makeLatLongMap.cpp
  readInputImage.cpp
  resizeImage.cpp
  makeCubeMap.cpp
  main.cpp
  blurImage.cpp
  EnvmapImage.cpp
)

target_link_libraries(exrenvmap OpenEXR::OpenEXR)
set_target_properties(exrenvmap PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
install(TARGETS exrenvmap DESTINATION ${CMAKE_INSTALL_BINDIR})
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrenvmap PRIVATE OPENEXR_DLL)
endif()
