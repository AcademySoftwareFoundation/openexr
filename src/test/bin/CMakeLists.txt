# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

if(BUILD_TESTING)

  find_package(Python3 COMPONENTS Interpreter)
  if (NOT Python3_FOUND)
    message(STATUS "Python3 not found: skipping bin tests")
    return()
  endif()

  message(STATUS "adding bin tests")

  set(TEST_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test_images")

  add_test(NAME OpenEXR.bin.exrinfo
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrinfo.py
           $<TARGET_FILE:exrinfo> ${TEST_IMAGE_DIR} ${OPENEXR_VERSION})
  set_tests_properties(OpenEXR.bin.exrinfo PROPERTIES LABELS "OpenEXR.bin")

  add_test(NAME OpenEXR.bin.exrheader
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrheader.py
           $<TARGET_FILE:exrheader> ${TEST_IMAGE_DIR} ${OPENEXR_VERSION})
  set_tests_properties(OpenEXR.bin.exrheader PROPERTIES LABELS "OpenEXR.bin")

  set(tests
      exr2aces
      exrenvmap
      exrmakepreview
      exrmaketiled
      exrmanifest
      exrmultiview
      exrmultipart
      exrstdattr
      exrmetrics
      exrperf
  )

  foreach(test ${tests})
    add_test(NAME OpenEXR.bin.${test}
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.py
           $<TARGET_FILE:${test}> $<TARGET_FILE:exrinfo>
           ${TEST_IMAGE_DIR} ${OPENEXR_VERSION})
    set_tests_properties(OpenEXR.bin.${test} PROPERTIES LABELS "OpenEXR.bin")
  endforeach()

  # run exrcheck on test images
  set(images
      ${TEST_IMAGE_DIR}/GrayRampsHorizontal.exr
      ${TEST_IMAGE_DIR}/WavyLinesLatLong.exr
      ${TEST_IMAGE_DIR}/WavyLinesCube.exr
      ${TEST_IMAGE_DIR}/Flowers.exr
      ${TEST_IMAGE_DIR}/GammaChart.exr
      ${TEST_IMAGE_DIR}/multipart.0001.exr
  )
  add_test(NAME OpenEXR.bin.exrcheck
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrcheck.py
           $<TARGET_FILE:exrcheck>
           ${images})
  set_tests_properties(OpenEXR.bin.exrcheck PROPERTIES LABELS "OpenEXR.bin")

endif()


