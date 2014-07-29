# yue.nicholas@gmail.com

# IlmImfTest

ADD_EXECUTABLE ( IlmImfTest
  compareB44.cpp
  compareDwa.cpp
  compareFloat.cpp
  main.cpp
  testAttributes.cpp
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
  testExistingStreams.cpp
  testFutureProofing.cpp
  testHuf.cpp
  testInputPart.cpp
  testIsComplete.cpp
  testLineOrder.cpp
  testLut.cpp
  testMagic.cpp
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


ADD_TEST ( TestIlmImf IlmImfTest )
TARGET_LINK_LIBRARIES ( IlmImfTest 
        IlmImf  
        Half 
        Iex${ILMBASE_LIBSUFFIX}
        Imath${ILMBASE_LIBSUFFIX}
        IlmThread${ILMBASE_LIBSUFFIX}
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
        )

