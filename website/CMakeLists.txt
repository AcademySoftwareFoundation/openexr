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

set(TEST_IMAGE_INDEX_FILE ${PROJECT_SOURCE_DIR}/website/_test_images/index.rst)
set(TEST_IMAGE_PY_FILE ${PROJECT_SOURCE_DIR}/website/scripts/test_images.py)
set(TEST_IMAGE_INPUT_FILE ${PROJECT_SOURCE_DIR}/website/test_images.txt)
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

add_custom_target(website ALL DEPENDS
                 ${SPHINX_INDEX_FILE}
                 ${DOXYGEN_INDEX_FILE}
                 ${TEST_IMAGE_INDEX_FILE})

