# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# If the project name is set, this is being configured as a part of
# OpenEXR.  If there is no project name, it's being configured as a
# standalone program linking against an already-installed OpenEXR
# library.

if("${CMAKE_PROJECT_NAME}" STREQUAL "")
  cmake_minimum_required(VERSION 3.12)
  project(OpenEXRExamples)
  find_package(OpenEXR)
endif()

add_executable(OpenEXRExamples
  drawImage.cpp
  generalInterfaceExamples.cpp
  generalInterfaceTiledExamples.cpp
  lowLevelIoExamples.cpp
  main.cpp
  previewImageExamples.cpp
  rgbaInterfaceExamples.cpp
  rgbaInterfaceTiledExamples.cpp
)
target_link_libraries(OpenEXRExamples OpenEXR::OpenEXR)

# Examples
install(
  FILES
    drawImage.cpp
    drawImage.h
    generalInterfaceExamples.cpp
    generalInterfaceExamples.h
    generalInterfaceTiledExamples.cpp
    generalInterfaceTiledExamples.h
    lowLevelIoExamples.cpp
    lowLevelIoExamples.h
    main.cpp
    namespaceAlias.h
    previewImageExamples.cpp
    previewImageExamples.h
    rgbaInterfaceExamples.cpp
    rgbaInterfaceExamples.h
    rgbaInterfaceTiledExamples.cpp
    rgbaInterfaceTiledExamples.h
  DESTINATION
    ${CMAKE_INSTALL_DOCDIR}/examples
  )
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(OpenEXRExamples PRIVATE OPENEXR_DLL)
endif()
