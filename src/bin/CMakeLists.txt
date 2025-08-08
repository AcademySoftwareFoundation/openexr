# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

##################
# Binaries / Tools
##################

message(STATUS "Building OpenEXR tools")

function(add_openexr_bin_program bin_name)
  cmake_parse_arguments(ARG "" "INSTALL_CONDITION" "SOURCES" ${ARGN})

  # _install_condition is OPENEXR_INSTALL_TOOLS by default, or
  # the remaining unparsed args if there are any, i.e.
  # OPENEXR_INSTALL_DEVELOPER_TOOLS for exrcheck
  set(_install_condition OPENEXR_INSTALL_TOOLS)
  if (ARG_INSTALL_CONDITION)
    set(_install_condition "${ARG_INSTALL_CONDITION}")
  endif()

  add_executable(${bin_name} ${ARG_SOURCES})
  target_link_libraries(${bin_name} OpenEXR::OpenEXR OpenEXR::OpenEXRUtil)
  set_target_properties(${bin_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  if(${_install_condition})
    install(TARGETS ${bin_name} DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  if(WIN32 AND BUILD_SHARED_LIBS)
    target_compile_definitions(${bin_name} PRIVATE OPENEXR_DLL)
  endif()

  # Compile with the indicated C++ standard
  target_compile_features(${bin_name} PRIVATE cxx_std_${OPENEXR_CXX_STANDARD})

endfunction()

add_subdirectory( exr2aces )
add_subdirectory( exrheader )
add_subdirectory( exrinfo )
add_subdirectory( exrmaketiled )
add_subdirectory( exrmetrics )
add_subdirectory( exrstdattr )
add_subdirectory( exrmakepreview )
add_subdirectory( exrenvmap )
add_subdirectory( exrmultiview )
add_subdirectory( exrmultipart )
add_subdirectory( exrcheck )
add_subdirectory( exrmanifest )
