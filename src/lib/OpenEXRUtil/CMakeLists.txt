# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

openexr_define_library(OpenEXRUtil
  PRIV_EXPORT OPENEXRUTIL_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    ImfImageChannel.cpp
    ImfFlatImageChannel.cpp
    ImfDeepImageChannel.cpp
    ImfSampleCountChannel.cpp
    ImfImageLevel.cpp
    ImfFlatImageLevel.cpp
    ImfDeepImageLevel.cpp
    ImfImage.cpp
    ImfFlatImage.cpp
    ImfDeepImage.cpp
    ImfImageIO.cpp
    ImfFlatImageIO.cpp
    ImfDeepImageIO.cpp
    ImfImageDataWindow.cpp
    ImfCheckFile.cpp
  HEADERS
    ImfImageChannel.h
    ImfFlatImageChannel.h
    ImfDeepImageChannel.h
    ImfSampleCountChannel.h
    ImfImageLevel.h
    ImfFlatImageLevel.h
    ImfDeepImageLevel.h
    ImfImage.h
    ImfFlatImage.h
    ImfDeepImage.h
    ImfImageIO.h
    ImfFlatImageIO.h
    ImfDeepImageIO.h
    ImfImageDataWindow.h
    ImfImageChannelRenaming.h
    ImfUtilExport.h
    ImfCheckFile.h
  DEPENDENCIES
    OpenEXR::OpenEXR
)
