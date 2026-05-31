# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project

add_executable(website_src

    C_IStream_read.cpp
    C_IStream_seekg.cpp
    C_IStream_tellg.cpp
    C_IStream_clear.cpp

    all.cpp
)

target_link_libraries(website_src OpenEXR::OpenEXR)

set_target_properties(website_src PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
