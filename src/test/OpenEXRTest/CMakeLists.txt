# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(OpenEXRTest
  TestUtilFStream.h
  bswap_32.h
  compareB44.cpp
  compareB44.h
  compareDwa.cpp
  compareDwa.h
  compareLJ2K.cpp
  compareLJ2K.h
  compareFloat.cpp
  compareFloat.h
  main.cpp
  random.cpp
  random.h
  testAttributes.cpp
  testAttributes.h
  testBackwardCompatibility.cpp
  testBackwardCompatibility.h
  testBadTypeAttributes.cpp
  testBadTypeAttributes.h
  testChannels.cpp
  testChannels.h
  testCompositeDeepScanLine.cpp
  testCompositeDeepScanLine.h
  testCompressionApi.cpp
  testCompressionApi.h
  testCompression.cpp
  testCompression.h
  testConversion.cpp
  testConversion.h
  testCopyDeepScanLine.cpp
  testCopyDeepScanLine.h
  testCopyDeepTiled.cpp
  testCopyDeepTiled.h
  testCopyMultiPartFile.cpp
  testCopyMultiPartFile.h
  testCopyPixels.cpp
  testCopyPixels.h
  testCpuId.cpp
  testCpuId.h
  testCustomAttributes.cpp
  testCustomAttributes.h
  testDeepScanLineBasic.cpp
  testDeepScanLineBasic.h
  testDeepScanLineHuge.cpp
  testDeepScanLineHuge.h
  testDeepScanLineMultipleRead.cpp
  testDeepScanLineMultipleRead.h
  testDeepTiledBasic.cpp
  testDeepTiledBasic.h
  testExistingStreams.cpp
  testExistingStreams.h
  testFutureProofing.cpp
  testFutureProofing.h
  testHeader.cpp
  testHeader.h
  testHuf.cpp
  testHuf.h
  testIDManifest.cpp
  testIDManifest.h
  testInputPart.cpp
  testInputPart.h
  testIsComplete.cpp
  testIsComplete.h
  testLargeDataWindowOffsets.cpp
  testLargeDataWindowOffsets.h
  testLineOrder.cpp
  testLineOrder.h
  testLut.cpp
  testLut.h
  testMagic.cpp
  testMagic.h
  testMalformedImages.cpp
  testMalformedImages.h
  testMultiPartApi.cpp
  testMultiPartApi.h
  testMultiPartFileMixingBasic.cpp
  testMultiPartFileMixingBasic.h
  testMultiPartSharedAttributes.cpp
  testMultiPartSharedAttributes.h
  testMultiPartThreading.cpp
  testMultiPartThreading.h
  testMultiScanlinePartThreading.cpp
  testMultiScanlinePartThreading.h
  testMultiTiledPartThreading.cpp
  testMultiTiledPartThreading.h
  testMultiView.cpp
  testMultiView.h
  testNativeFormat.cpp
  testNativeFormat.h
  testOptimizedInterleavePatterns.cpp
  testOptimizedInterleavePatterns.h
  testPartHelper.cpp
  testPartHelper.h
  testPreviewImage.cpp
  testPreviewImage.h
  testRgba.cpp
  testRgba.h
  testCRgba.cpp
  testCRgba.h
  testRgbaThreading.cpp
  testRgbaThreading.h
  testRle.cpp
  testRle.h
  testSampleImages.cpp
  testSampleImages.h
  testScanLineApi.cpp
  testScanLineApi.h
  testSharedFrameBuffer.cpp
  testSharedFrameBuffer.h
  testStandardAttributes.cpp
  testStandardAttributes.h
  testTiledCompression.cpp
  testTiledCompression.h
  testLJ2KBoundary.cpp
  testLJ2KBoundary.h
  testTiledCopyPixels.cpp
  testTiledCopyPixels.h
  testTiledLineOrder.cpp
  testTiledLineOrder.h
  testTiledRgba.cpp
  testTiledRgba.h
  testTiledYa.cpp
  testTiledYa.h
  testWav.cpp
  testWav.h
  testXdr.cpp
  testXdr.h
  testYca.cpp
  testYca.h
  tmpDir.h
)
target_compile_definitions(OpenEXRTest PRIVATE ILM_IMF_TEST_IMAGEDIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(OpenEXRTest OpenEXR::OpenEXR)
set_target_properties(OpenEXRTest PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(OpenEXRTest PRIVATE OPENEXR_DLL)
endif()
target_compile_features(OpenEXRTest
    PRIVATE cxx_std_${OPENEXR_CXX_STANDARD}
)

function(DEFINE_OPENEXR_TESTS)
  foreach(curtest IN LISTS ARGN)
    # CMAKE_CROSSCOMPILING_EMULATOR is necessary to support cross-compiling (ex: to win32 from mingw and running tests with wine)
    add_test(NAME OpenEXR.${curtest} COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:OpenEXRTest> ${curtest})
    set_tests_properties(OpenEXR.${curtest} PROPERTIES LABELS "OpenEXR")
  endforeach()
endfunction()

define_openexr_tests(
 testAttributes
 testBackwardCompatibility
 testBadTypeAttributes
 testChannels
 testCompositeDeepScanLine
 testCompressionApi
 testCompression
 testConversion
 testCopyDeepScanLine
 testCopyDeepTiled
 testCopyMultiPartFile
 testCopyPixels
 testCpuId
 testCustomAttributes
 testDeepScanLineBasic
 testDeepScanLineMultipleRead
 testDeepTiledBasic
 testDwaLookups
 testExistingStreams
 testExistingStreamsUTF8
 testFutureProofing
 testHeader
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
 testOptimizedInterleavePatterns
 testPartHelper
 testPreviewImage
 testRgba
 testCRgba
 testRgbaThreading
 testRle
 testSampleImages
 testScanLineApi
 testSharedFrameBuffer
 testStandardAttributes
 testTiledCompression
 testLJ2KBoundary
 testTiledCopyPixels
 testTiledLineOrder
 testTiledRgba
 testTiledYa
 testWav
 testXdr
 testYca
 testIDManifest
)

# "portability" tests: an abbreviated subset of the test suite run by
# specialty CI jobs (alternate OS/build-variant jobs) to test the
# behaviors most likely to differ across OS/toolchain/libc.
# Primary Linux/macOS/Windows jobs run the complete test suite.
set_tests_properties(
  OpenEXR.testCpuId
  OpenEXR.testNativeFormat
  OpenEXR.testExistingStreams
  OpenEXR.testExistingStreamsUTF8
  OpenEXR.testMagic
  OpenEXR.testBackwardCompatibility
  OpenEXR.testPreviewImage
  OpenEXR.testIsComplete
  OpenEXR.testRgba
  OpenEXR.testMultiPartThreading
  OpenEXR.testMultiScanlinePartThreading
  OpenEXR.testRgbaThreading
  OpenEXR.testSharedFrameBuffer
  PROPERTIES LABELS "OpenEXR;portability"
)

# These tests configure their own dedicated ThreadPool(s) (in addition to,
# or instead of, the library's default global pool) and so can use many
# more OS threads than a typical test. Tell ctest's --parallel scheduler
# how many "slots" each test needs so it won't pack additional tests onto
# the same cores and cause CPU oversubscription/noisy timing; with no
# --parallel (serial ctest), PROCESSORS has no effect and these tests run
# exactly as before.
set_tests_properties (OpenEXR.testMultiPartThreading PROPERTIES PROCESSORS 32)
set_tests_properties (
  OpenEXR.testMultiScanlinePartThreading
  OpenEXR.testMultiTiledPartThreading
  PROPERTIES PROCESSORS 2
)

