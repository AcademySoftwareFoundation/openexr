# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_openexr_bin_program(exrenvmap SOURCES
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
