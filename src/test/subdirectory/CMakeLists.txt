# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

#
# Test project to validate that add_subdirectory(openexr) works
#

cmake_minimum_required(VERSION 3.14)
project(exrwriter)

add_subdirectory(ext/openexr)

add_executable(exrwriter exrwriter.cpp)

target_link_libraries(exrwriter PRIVATE OpenEXR::OpenEXR)
