# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(OpenEXRTest
  compareB44.cpp
  compareDwa.cpp
  compareFloat.cpp
  main.cpp
  random.cpp
  testAttributes.cpp
  testB44ExpLogTable.cpp
  testBackwardCompatibility.cpp
  testBadTypeAttributes.cpp
  testChannels.cpp
  testCompositeDeepScanLine.cpp
  testCompression.cpp
  testConversion.cpp
  testCopyDeepScanLine.cpp
  testCopyDeepTiled.cpp
  testCopyMultiPartFile.cpp
  testCopyPixels.cpp
  testCustomAttributes.cpp
  testDeepScanLineBasic.cpp
  testDeepScanLineHuge.cpp
  testDeepScanLineMultipleRead.cpp
  testDeepTiledBasic.cpp
  testDwaCompressorSimd.cpp
  testDwaLookups.cpp
  testExistingStreams.cpp
  testFutureProofing.cpp
  testHuf.cpp
  testIDManifest.cpp
  testInputPart.cpp
  testIsComplete.cpp
  testLargeDataWindowOffsets.cpp
  testLineOrder.cpp
  testLut.cpp
  testMagic.cpp
  testMalformedImages.cpp
  testMultiPartApi.cpp
  testMultiPartFileMixingBasic.cpp
  testMultiPartSharedAttributes.cpp
  testMultiPartThreading.cpp
  testMultiScanlinePartThreading.cpp
  testMultiTiledPartThreading.cpp
  testMultiView.cpp
  testNativeFormat.cpp
  testOptimized.cpp
  testOptimizedInterleavePatterns.cpp
  testPartHelper.cpp
  testPreviewImage.cpp
  testRgba.cpp
  testRgbaThreading.cpp
  testRle.cpp
  testSampleImages.cpp
  testScanLineApi.cpp
  testSharedFrameBuffer.cpp
  testStandardAttributes.cpp
  testTiledCompression.cpp
  testTiledCopyPixels.cpp
  testTiledLineOrder.cpp
  testTiledRgba.cpp
  testTiledYa.cpp
  testWav.cpp
  testXdr.cpp
  testYca.cpp
)
target_compile_definitions(OpenEXRTest PRIVATE ILM_IMF_TEST_IMAGEDIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(OpenEXRTest OpenEXR::OpenEXR)
set_target_properties(OpenEXRTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(OpenEXRTest PRIVATE OPENEXR_DLL)
endif()

function(DEFINE_OPENEXR_TESTS)
  foreach(curtest IN LISTS ARGN)
    # CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
    add_test(NAME OpenEXR.${curtest} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:OpenEXRTest> ${curtest})
  endforeach()
endfunction()

define_openexr_tests(
 testAttributes
 testB44ExpLogTable
 testBackwardCompatibility
 testBadTypeAttributes
 testChannels
 testCompositeDeepScanLine
 testCompression
 testConversion
 testCopyDeepScanLine
 testCopyDeepTiled
 testCopyMultiPartFile
 testCopyPixels
 testCustomAttributes
 testDeepScanLineBasic
 testDeepScanLineMultipleRead
 testDeepTiledBasic
 testDwaCompressorSimd
 testDwaLookups
 testExistingStreams
 testFutureProofing
 testHuf
 testInputPart
 testIsComplete
 testLargeDataWindowOffsets
 testLineOrder
 testLut
 testMagic
 testMultiPartApi
 testMultiPartFileMixingBasic
 testMultiPartSharedAttributes
 testMultiPartThreading
 testMultiScanlinePartThreading
 testMultiTiledPartThreading
 testMultiView
 testNativeFormat
 testOptimized
 testOptimizedInterleavePatterns
 testPartHelper
 testPreviewImage
 testRgba
 testRgbaThreading
 testRle
 testSampleImages
 testScanLineApi
 testSharedFrameBuffer
 testStandardAttributes
 testTiledCompression
 testTiledCopyPixels
 testTiledLineOrder
 testTiledRgba
 testTiledYa
 testWav
 testXdr
 testYca
 testIDManifest
)
