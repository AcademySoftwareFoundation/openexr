# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)
target_link_libraries(exrmultiview OpenEXR::IlmImf)
install(TARGETS exrmultiview DESTINATION ${CMAKE_INSTALL_BINDIR})
