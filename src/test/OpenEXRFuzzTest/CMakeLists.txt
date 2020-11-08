# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

option(OPENEXR_RUN_FUZZ_TESTS  "Controls whether to include the fuzz tests (slow) in default test cases, there is a custom fuzz target" OFF)
if(OPENEXR_RUN_FUZZ_TESTS)
  add_executable( OpenEXRFuzzTest
    fuzzFile.cpp
    main.cpp
    testFuzzDeepTiles.cpp
    testFuzzDeepScanLines.cpp
    testFuzzScanLines.cpp
    testFuzzTiles.cpp
  )
  target_link_libraries(OpenEXRFuzzTest OpenEXR::OpenEXR)
  set_target_properties(OpenEXRFuzzTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )

function(DEFINE_OPENEXR_FUZZ_TESTS)
  foreach(curtest IN LISTS ARGN)
    add_test(NAME OpenEXR.Fuzz.${curtest} COMMAND $<TARGET_FILE:OpenEXRFuzzTest> ${curtest})
    set_tests_properties(OpenEXR.Fuzz.${curtest} PROPERTIES TIMEOUT 36000)
  endforeach()
endfunction()
  define_openexr_fuzz_tests(
    testFuzzScanLines
    testFuzzTiles
    testFuzzDeepScanLines
    testFuzzDeepTiles
  )
  #add_test(NAME OpenEXR.Fuzz COMMAND $<TARGET_FILE:OpenEXRFuzzTest>)
  #set_tests_properties(OpenEXR.Fuzz PROPERTIES TIMEOUT 36000)

  add_custom_target(fuzz $<TARGET_FILE:OpenEXRFuzzTest>)
  add_dependencies(fuzz OpenEXRFuzzTest)
endif()
