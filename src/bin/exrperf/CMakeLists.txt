# SPDX-License-Identifier: BSD-3-Clause

add_executable(exrperf main.cpp)
target_link_libraries(exrperf OpenEXR::OpenEXR)
set_target_properties(exrperf PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrperf DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrperf PRIVATE OPENEXR_DLL)
endif()
