# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrstdattr main.cpp)
target_link_libraries(exrstdattr OpenEXR::IlmImf)
install(TARGETS exrstdattr DESTINATION ${CMAKE_INSTALL_BINDIR})
