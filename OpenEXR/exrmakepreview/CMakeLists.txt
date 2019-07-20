# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmakepreview
  main.cpp
  makePreview.cpp
)
target_link_libraries(exrmakepreview OpenEXR::IlmImf)
install(TARGETS exrmakepreview DESTINATION ${CMAKE_INSTALL_BINDIR})
