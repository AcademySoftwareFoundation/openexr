# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_openexr_bin_program(exrcheck
  SOURCES main.cpp
  INSTALL_CONDITION OPENEXR_INSTALL_DEVELOPER_TOOLS
)
