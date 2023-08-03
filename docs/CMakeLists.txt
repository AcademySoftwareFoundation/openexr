# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

include(FindPythonPackage)
find_python_package(sphinx-press-theme REQUIRED)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src/lib/OpenEXRCore)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

set(SPHINX_OPTS "-W" CACHE STRING "Sphinx build options")

set(OPENEXR_TEST_IMAGE_REPO "https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images" 
    CACHE STRING "Repo for test images")
set(OPENEXR_TEST_IMAGE_TAG "main" 
    CACHE STRING "Tag for test image repo")

set(TEST_IMAGE_INDEX_FILE ${PROJECT_SOURCE_DIR}/docs/_test_images/index.rst)
set(TEST_IMAGE_PY_FILE ${PROJECT_SOURCE_DIR}/docs/scripts/test_images.py)
set(TEST_IMAGE_INPUT_FILE ${PROJECT_SOURCE_DIR}/docs/test_images.txt)
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Running doxygen"
                   VERBATIM)

add_custom_command(OUTPUT ${TEST_IMAGE_INDEX_FILE}
                   # run the test_images.py script with the PATH set
                   # to include the build's bin directory, to pick up
                   # the local bin/exrheader executable
                   COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_CURRENT_BINARY_DIR}/../bin:$ENV{PATH}"
                   ${Python3_EXECUTABLE} ${TEST_IMAGE_PY_FILE} ${OPENEXR_TEST_IMAGE_REPO} ${OPENEXR_TEST_IMAGE_TAG} 
                   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                   DEPENDS
                   ${TEST_IMAGE_PY_FILE} ${TEST_IMAGE_INPUT_FILE}
                   COMMENT "Generating test image pages"
                   VERBATIM)

add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND 
                     ${SPHINX_EXECUTABLE} -b html ${SPHINX_OPTS}
                     # Tell Breathe where to find the Doxygen output
                     -Dbreathe_projects.OpenEXR=${DOXYGEN_OUTPUT_DIR}/xml
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS ${DOXYGEN_INDEX_FILE} ${TEST_IMAGE_INDEX_FILE} ${TEST_IMAGE_PY_FILE} ${TEST_IMAGE_INPUT_FILE} 
                   MAIN_DEPENDENCY ${SPHINX_CONF_PY_FILE}
                   COMMENT "Generating documentation with Sphinx")


# man pages for the bin programs

find_program(HELP2MAN help2man)

set(MANPAGE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/man)
file(MAKE_DIRECTORY ${MANPAGE_OUTPUT_DIR}/man1)

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
define_manpage(exrmultipart   "combine or split multipart exr images")
define_manpage(exrmultiview   "convert between single/multi-view exr images")
define_manpage(exrstdattr     "set exr image metadata")

add_subdirectory(src)

add_custom_target(docs ALL DEPENDS
                 ${SPHINX_INDEX_FILE}
                 ${DOXYGEN_INDEX_FILE}
                 ${TEST_IMAGE_INDEX_FILE}
                 ${manpage_files}
                 ${CMAKE_BINARY_DIR}/bin/docs_src)

# Add a target to install the manpages, but *not* the html

if(INSTALL_DOCS)
    include(GNUInstallDirs)
    install(DIRECTORY ${MANPAGE_OUTPUT_DIR}/man1 DESTINATION ${CMAKE_INSTALL_MANDIR})
endif()

# build the example code to verify that it compiles


