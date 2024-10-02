# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

if(BUILD_TESTING)

  find_package(Python3 COMPONENTS Interpreter)
  if (NOT Python3_FOUND)
    message(STATUS "Python3 not found: skipping bin tests")
    return()
  endif()

  message(STATUS "adding bin tests")

  set(OPENEXR_IMAGES_REPO "https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images" CACHE STRING
    "Bin test Image repo")

  set(OPENEXR_IMAGES_TAG "v1.0" CACHE STRING "Bin test image repo tag")

  if ("${OPENEXR_IMAGES_TAG}" STREQUAL "")
    set(images_url ${OPENEXR_IMAGES_REPO})
  else()
    set(images_url ${OPENEXR_IMAGES_REPO}/${OPENEXR_IMAGES_TAG})
  endif()

  set(images
      TestImages/GrayRampsHorizontal.exr
      LuminanceChroma/Garden.exr
      MultiResolution/ColorCodedLevels.exr
      MultiResolution/WavyLinesLatLong.exr
      MultiResolution/WavyLinesCube.exr
      LuminanceChroma/Flowers.exr
      MultiView/Adjuster.exr
      Chromaticities/Rec709_YC.exr
      Chromaticities/Rec709.exr
      Chromaticities/XYZ_YC.exr
      Chromaticities/XYZ.exr
      TestImages/GammaChart.exr
      Beachball/singlepart.0001.exr
      v2/LeftView/Balls.exr
      v2/Stereo/Trunks.exr
      Beachball/multipart.0001.exr
  )

  foreach(image ${images})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${image})
      message(STATUS "Downloading ${images_url}/${image}")
      file(DOWNLOAD ${images_url}/${image} ${CMAKE_CURRENT_BINARY_DIR}/${image})
    endif()
  endforeach()

  add_test(NAME OpenEXR.bin.exrinfo
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrinfo.py
           $<TARGET_FILE:exrinfo> ${CMAKE_CURRENT_BINARY_DIR} ${OPENEXR_VERSION})

  add_test(NAME OpenEXR.bin.exrheader
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrheader.py
           $<TARGET_FILE:exrheader> ${CMAKE_CURRENT_BINARY_DIR} ${OPENEXR_VERSION})

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
  )

  foreach(test ${tests})
    add_test(NAME OpenEXR.bin.${test}
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.py
           $<TARGET_FILE:${test}> $<TARGET_FILE:exrinfo>
           ${CMAKE_CURRENT_BINARY_DIR} ${OPENEXR_VERSION})
  endforeach()

  # run exrcheck on all images
  add_test(NAME OpenEXR.bin.exrcheck
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrcheck.py
           $<TARGET_FILE:exrcheck>
           ${CMAKE_CURRENT_BINARY_DIR}
           ${images})

endif()


