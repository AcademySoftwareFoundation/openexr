# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# man pages for the bin programs

find_program(HELP2MAN help2man)

set(MANPAGE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/man)
file(MAKE_DIRECTORY ${MANPAGE_OUTPUT_DIR}/man1)

message(STATUS "MANPAGE_OUTPUT_DIR=${MANPAGE_OUTPUT_DIR}")
message(STATUS "CMAKE_INSTALL_MANDIR=${CMAKE_INSTALL_MANDIR}")

set(manpage_files "")

function(DEFINE_MANPAGE tool description)

    set(manpage_file ${MANPAGE_OUTPUT_DIR}/man1/${tool}.1)
    set(manpage_files "${manpage_files};${manpage_file}" PARENT_SCOPE)

    add_custom_command(OUTPUT ${manpage_file}
                   COMMAND
                   ${HELP2MAN} -n ${description} --output=${manpage_file} -N
                   ${CMAKE_CURRENT_BINARY_DIR}/../bin/${tool} 
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../bin/${tool}
                   COMMENT "Generating man page for ${tool}"
                   VERBATIM)
endfunction()

# Provide help2man a useful single-line program description. There's
# no especially good way to embed this in the program's --help text so
# that help2man can detect it.

define_manpage(exr2aces       "convert exr images to ACES format")
define_manpage(exrcheck       "validate exr files")
define_manpage(exrenvmap      "convert exr image environment  maps")
define_manpage(exrheader      "print exr image header metadata")
define_manpage(exrinfo        "print exr image header metadata")
define_manpage(exrmakepreview "generate exr preview thumbnail images")
define_manpage(exrmaketiled   "convert exr images to tiled format")
define_manpage(exrmanifest    "print id manifest")
define_manpage(exrmetrics     "report read/write time")
define_manpage(exrmultipart   "combine or split multipart exr images")
define_manpage(exrmultiview   "convert between single/multi-view exr images")
define_manpage(exrstdattr     "set exr image metadata")

add_custom_target(docs ALL DEPENDS ${manpage_files})

# Add a target to install the manpages

include(GNUInstallDirs)
install(DIRECTORY ${MANPAGE_OUTPUT_DIR}/man1 DESTINATION ${CMAKE_INSTALL_MANDIR})



