# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(website_src all.cpp)
target_link_libraries(website_src OpenEXR::OpenEXR)
set_target_properties(website_src PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
