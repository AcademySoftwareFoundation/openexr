# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src/lib/OpenEXRCore)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

set(SPHINX_OPTS "-W" CACHE STRING "Sphinx build options")

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Running doxygen"
                   VERBATIM)

add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND 
                     ${SPHINX_EXECUTABLE} -b html ${SPHINX_OPTS}
                     # Tell Breathe where to find the Doxygen output
                     -Dbreathe_projects.OpenEXR=${DOXYGEN_OUTPUT_DIR}/xml
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS ${DOXYGEN_INDEX_FILE} 
                   MAIN_DEPENDENCY ${SPHINX_CONF_PY_FILE}
                   COMMENT "Generating documentation with Sphinx")

add_custom_target(website ALL DEPENDS
                 ${SPHINX_INDEX_FILE}
                 ${DOXYGEN_INDEX_FILE})

