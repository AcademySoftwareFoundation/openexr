# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrmaketiled 
  makeTiled.cpp
  main.cpp
  Image.cpp
)
target_link_libraries(exrmaketiled OpenEXR::IlmImf)
install(TARGETS exrmaketiled DESTINATION ${CMAKE_INSTALL_BINDIR})
