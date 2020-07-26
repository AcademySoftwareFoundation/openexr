# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(HalfTest
  main.cpp
  testArithmetic.cpp
  testBitPatterns.cpp
  testClassification.cpp
  testError.cpp
  testFunction.cpp
  testLimits.cpp
  testSize.cpp
  testToFloat.cpp
)

target_link_libraries(HalfTest IlmBase::Half)
set_target_properties(HalfTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(HalfTest PRIVATE OPENEXR_DLL)
endif()
add_test(NAME IlmBase.Half COMMAND $<TARGET_FILE:HalfTest>)
