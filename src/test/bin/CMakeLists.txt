# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

if(BUILD_TESTING)

  find_package(Python3 COMPONENTS Interpreter)
  if (NOT Python3_FOUND)
    message(STATUS "Python3 not found: skipping bin tests")
    return()
  endif()
    
  message(STATUS "adding bin tests")

  set(images_url "https://raw.githubusercontent.com/AcademySoftwareFoundation/openexr-images/main")
  set(images 
      TestImages/GrayRampsHorizontal.exr
      LuminanceChroma/Garden.exr
      MultiResolution/ColorCodedLevels.exr
      MultiResolution/WavyLinesLatLong.exr
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
      message(STATUS "Downloading ${images_url}/${image}")
      file(DOWNLOAD ${images_url}/${image} ${CMAKE_CURRENT_BINARY_DIR}/${image})
  endforeach()

  add_test(NAME OpenEXR.bin.exrinfo
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrinfo.py $<TARGET_FILE:exrinfo> ${CMAKE_CURRENT_BINARY_DIR})

  add_test(NAME OpenEXR.bin.exrheader
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrheader.py $<TARGET_FILE:exrheader> ${CMAKE_CURRENT_BINARY_DIR})

  set(tests
      exr2aces
      exrenvmap
      exrmakepreview
      exrmaketiled
      exrmultiview
      exrmultipart
      exrstdattr
  )

  foreach(test ${tests})
  
    add_test(NAME OpenEXR.bin.${test}
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.py $<TARGET_FILE:${test}> $<TARGET_FILE:exrinfo> ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()
  
  # run exrcheck on all images
  add_test(NAME OpenEXR.bin.exrcheck
           COMMAND ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/test_exrcheck.py
           $<TARGET_FILE:exrcheck>
           ${CMAKE_CURRENT_BINARY_DIR}
           ${images})

endif()


