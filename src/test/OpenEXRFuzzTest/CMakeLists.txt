# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

option(OPENEXR_RUN_FUZZ_TESTS  "Controls whether to include the fuzz tests (slow) in default test cases, there is a custom fuzz target" OFF)
if(OPENEXR_RUN_FUZZ_TESTS)
  add_executable( OpenEXRFuzzTest
    fuzzFile.cpp
    fuzzFile.h
    main.cpp
    testFuzzDeepScanLines.cpp
    testFuzzDeepScanLines.h
    testFuzzDeepTiles.cpp
    testFuzzDeepTiles.h
    testFuzzScanLines.cpp
    testFuzzScanLines.h
    testFuzzTiles.cpp
    testFuzzTiles.h
  )
  target_include_directories(OpenEXRFuzzTest PRIVATE ../OpenEXRTest)
  target_link_libraries(OpenEXRFuzzTest OpenEXR::OpenEXR)
  set_target_properties(OpenEXRFuzzTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

function(DEFINE_OPENEXR_FUZZ_TESTS)
  foreach(curtest IN LISTS ARGN)
    # CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
    add_test(NAME OpenEXR.Fuzz.${curtest} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:OpenEXRFuzzTest> ${curtest})
    set_tests_properties(OpenEXR.Fuzz.${curtest} PROPERTIES TIMEOUT 36000)
  endforeach()
endfunction()
  define_openexr_fuzz_tests(
    testFuzzScanLines
    testFuzzTiles
    testFuzzDeepScanLines
    testFuzzDeepTiles
  )

  add_custom_target(fuzz $<TARGET_FILE:OpenEXRFuzzTest>)
  add_dependencies(fuzz OpenEXRFuzzTest)
endif()
