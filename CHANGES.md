# OpenEXR Release Notes

## Version 2.3.0:

### Features/Improvements:

* ThreadPool overhead improvements, enable custom thread pool to be registered via ThreadPoolProvider class
* Fixes to enable custom namespaces for Iex, Imf
* Improve read performance for deep/zipped data, and SIMD-accelerated uncompress support
* Added rawPixelDataToBuffer() function for access to compressed scanlines
* Iex::BaseExc no longer derived from std::string.
* Imath throw() specifiers removed
* Initial Support for Python 3

### Bugs:

* 25+ various bug fixes (see detailed Release Notes for the full list)

### Build Fixes:

* Various fixes to the cmake and autoconf build infrastructures
* Various changes to support compiling for C++11 / C++14 / C++17 and GCC 6.3.1
* Various fixes to address Windows build issues
* 60+ total build-related fixes (see detailed Release Notes for the full list)

### Diff Stats \[git diff --stat v2.2.1\]

    CHANGES.md                                         |  1487 +++
    CMakeLists.txt                                     |   194 +
    Contrib/DtexToExr/bootstrap                        |     2 +-
    Contrib/DtexToExr/configure.ac                     |     2 +-
    IlmBase/CMakeLists.txt                             |   214 +-
    IlmBase/COPYING                                    |    34 -
    IlmBase/Half/CMakeLists.txt                        |   107 +-
    IlmBase/Half/half.cpp                              |     6 +-
    IlmBase/Half/half.h                                |     8 +-
    IlmBase/Half/halfExport.h                          |    44 +-
    IlmBase/Half/halfLimits.h                          |     9 +
    IlmBase/HalfTest/CMakeLists.txt                    |     4 +-
    IlmBase/HalfTest/testLimits.cpp                    |    13 +-
    IlmBase/INSTALL                                    |     2 -
    IlmBase/Iex/CMakeLists.txt                         |    81 +-
    IlmBase/Iex/IexBaseExc.cpp                         |    71 +-
    IlmBase/Iex/IexBaseExc.h                           |    87 +-
    IlmBase/Iex/IexMacros.h                            |    62 +-
    IlmBase/IexMath/CMakeLists.txt                     |    76 +-
    IlmBase/IexMath/IexMathFloatExc.cpp                |    18 +
    IlmBase/IexMath/IexMathFloatExc.h                  |    36 +-
    IlmBase/IexTest/CMakeLists.txt                     |     4 +-
    IlmBase/IexTest/testBaseExc.cpp                    |     2 +-
    IlmBase/IlmThread/CMakeLists.txt                   |    78 +-
    IlmBase/IlmThread/IlmThread.cpp                    |    48 +-
    IlmBase/IlmThread/IlmThread.h                      |    48 +-
    IlmBase/IlmThread/IlmThreadForward.h               |     8 +
    IlmBase/IlmThread/IlmThreadMutex.cpp               |     7 +-
    IlmBase/IlmThread/IlmThreadMutex.h                 |    48 +-
    IlmBase/IlmThread/IlmThreadMutexPosix.cpp          |    10 +-
    IlmBase/IlmThread/IlmThreadMutexWin32.cpp          |     9 +-
    IlmBase/IlmThread/IlmThreadPool.cpp                |   720 +-
    IlmBase/IlmThread/IlmThreadPool.h                  |    64 +-
    IlmBase/IlmThread/IlmThreadPosix.cpp               |     2 +
    IlmBase/IlmThread/IlmThreadSemaphore.h             |    49 +-
    .../IlmThread/IlmThreadSemaphorePosixCompat.cpp    |    78 +-
    IlmBase/IlmThread/IlmThreadWin32.cpp               |     6 +
    IlmBase/Imath/CMakeLists.txt                       |   130 +-
    IlmBase/Imath/ImathBox.cpp                         |    37 -
    IlmBase/Imath/ImathEuler.h                         |     7 +-
    IlmBase/Imath/ImathInt64.h                         |     3 +
    IlmBase/Imath/ImathMatrix.h                        |    56 +-
    IlmBase/Imath/ImathShear.cpp                       |    54 -
    IlmBase/Imath/ImathVec.cpp                         |    24 +-
    IlmBase/Imath/ImathVec.h                           |    48 +-
    IlmBase/Imath/Makefile.am                          |     4 +-
    IlmBase/ImathTest/CMakeLists.txt                   |     6 +-
    IlmBase/Makefile.am                                |     5 +-
    IlmBase/README                                     |    70 -
    IlmBase/README.CVS                                 |    16 -
    IlmBase/README.OSX                                 |   101 -
    IlmBase/README.cmake.txt                           |    65 -
    IlmBase/README.git                                 |    16 -
    IlmBase/README.md                                  |   277 +
    IlmBase/README.namespacing                         |    83 -
    IlmBase/bootstrap                                  |     4 +-
    IlmBase/config.windows/IlmBaseConfig.h             |     1 +
    IlmBase/config/IlmBaseConfig.h.in                  |     7 +
    IlmBase/configure.ac                               |    50 +-
    IlmBase/m4/ax_cxx_compile_stdcxx.m4                |   982 ++
    LICENSE                                            |    34 +
    OpenEXR/AUTHORS                                    |     2 +
    OpenEXR/CMakeLists.txt                             |   272 +-
    OpenEXR/COPYING                                    |    34 -
    OpenEXR/INSTALL                                    |     2 -
    OpenEXR/IlmImf/CMakeLists.txt                      |   396 +-
    OpenEXR/IlmImf/ImfAcesFile.h                       |    38 +-
    OpenEXR/IlmImf/ImfAttribute.cpp                    |     6 +
    OpenEXR/IlmImf/ImfAttribute.h                      |     8 +-
    OpenEXR/IlmImf/ImfB44Compressor.h                  |    10 +-
    OpenEXR/IlmImf/ImfCRgbaFile.h                      |     2 +-
    OpenEXR/IlmImf/ImfChannelList.h                    |    45 +-
    OpenEXR/IlmImf/ImfChromaticities.h                 |     5 +-
    OpenEXR/IlmImf/ImfCompositeDeepScanLine.h          |    14 +-
    OpenEXR/IlmImf/ImfCompressionAttribute.h           |     6 +-
    OpenEXR/IlmImf/ImfCompressor.h                     |    14 +-
    OpenEXR/IlmImf/ImfDeepCompositing.h                |     6 +-
    OpenEXR/IlmImf/ImfDeepFrameBuffer.h                |    38 +-
    OpenEXR/IlmImf/ImfDeepScanLineInputFile.cpp        |     6 +-
    OpenEXR/IlmImf/ImfDeepScanLineInputFile.h          |    20 +-
    OpenEXR/IlmImf/ImfDeepScanLineInputPart.h          |    18 +-
    OpenEXR/IlmImf/ImfDeepScanLineOutputFile.cpp       |    14 +-
    OpenEXR/IlmImf/ImfDeepScanLineOutputFile.h         |    14 +-
    OpenEXR/IlmImf/ImfDeepScanLineOutputPart.h         |    12 +-
    OpenEXR/IlmImf/ImfDeepTiledInputFile.cpp           |    16 +-
    OpenEXR/IlmImf/ImfDeepTiledInputFile.h             |    37 +-
    OpenEXR/IlmImf/ImfDeepTiledInputPart.cpp           |     2 +-
    OpenEXR/IlmImf/ImfDeepTiledInputPart.h             |    34 +-
    OpenEXR/IlmImf/ImfDeepTiledOutputFile.cpp          |    18 +-
    OpenEXR/IlmImf/ImfDeepTiledOutputFile.h            |    33 +-
    OpenEXR/IlmImf/ImfDeepTiledOutputPart.h            |    31 +-
    OpenEXR/IlmImf/ImfDwaCompressor.cpp                |   232 +-
    OpenEXR/IlmImf/ImfDwaCompressor.h                  |    43 +-
    OpenEXR/IlmImf/ImfDwaCompressorSimd.h              |    67 +-
    OpenEXR/IlmImf/ImfFastHuf.cpp                      |    62 +-
    OpenEXR/IlmImf/ImfFastHuf.h                        |     5 +
    OpenEXR/IlmImf/ImfFrameBuffer.h                    |    36 +-
    OpenEXR/IlmImf/ImfGenericInputFile.h               |     5 +-
    OpenEXR/IlmImf/ImfGenericOutputFile.h              |     6 +-
    OpenEXR/IlmImf/ImfHeader.h                         |    90 +-
    OpenEXR/IlmImf/ImfIO.h                             |    13 +-
    OpenEXR/IlmImf/ImfInputFile.cpp                    |    41 +-
    OpenEXR/IlmImf/ImfInputFile.h                      |    42 +-
    OpenEXR/IlmImf/ImfInputPart.cpp                    |     8 +
    OpenEXR/IlmImf/ImfInputPart.h                      |    22 +-
    OpenEXR/IlmImf/ImfInputPartData.h                  |     1 +
    OpenEXR/IlmImf/ImfInt64.h                          |     1 +
    OpenEXR/IlmImf/ImfKeyCode.h                        |    19 +-
    OpenEXR/IlmImf/ImfLut.h                            |     8 +-
    OpenEXR/IlmImf/ImfMisc.cpp                         |    55 +-
    OpenEXR/IlmImf/ImfMisc.h                           |    20 +-
    OpenEXR/IlmImf/ImfMultiPartInputFile.cpp           |     4 +-
    OpenEXR/IlmImf/ImfMultiPartInputFile.h             |    10 +-
    OpenEXR/IlmImf/ImfMultiPartOutputFile.cpp          |     4 +-
    OpenEXR/IlmImf/ImfMultiPartOutputFile.h            |    10 +-
    OpenEXR/IlmImf/ImfName.h                           |     9 +
    OpenEXR/IlmImf/ImfOpaqueAttribute.h                |    10 +-
    OpenEXR/IlmImf/ImfOptimizedPixelReading.h          |     4 +-
    OpenEXR/IlmImf/ImfOutputFile.cpp                   |    95 +-
    OpenEXR/IlmImf/ImfOutputFile.h                     |    15 +-
    OpenEXR/IlmImf/ImfOutputPart.h                     |    13 +-
    OpenEXR/IlmImf/ImfOutputPartData.h                 |    23 +-
    OpenEXR/IlmImf/ImfPizCompressor.h                  |    10 +-
    OpenEXR/IlmImf/ImfPreviewImage.h                   |    14 +-
    OpenEXR/IlmImf/ImfPxr24Compressor.h                |    10 +-
    OpenEXR/IlmImf/ImfRational.h                       |     3 +-
    OpenEXR/IlmImf/ImfRgbaFile.h                       |    47 +-
    OpenEXR/IlmImf/ImfRleCompressor.h                  |     7 +-
    OpenEXR/IlmImf/ImfScanLineInputFile.cpp            |    42 +-
    OpenEXR/IlmImf/ImfScanLineInputFile.h              |    37 +-
    OpenEXR/IlmImf/ImfSimd.h                           |    11 +-
    OpenEXR/IlmImf/ImfStdIO.cpp                        |    36 +-
    OpenEXR/IlmImf/ImfStdIO.h                          |    24 +-
    OpenEXR/IlmImf/ImfSystemSpecific.h                 |    15 +-
    OpenEXR/IlmImf/ImfTileOffsets.h                    |    16 +-
    OpenEXR/IlmImf/ImfTiledInputFile.cpp               |    16 +-
    OpenEXR/IlmImf/ImfTiledInputFile.h                 |    32 +-
    OpenEXR/IlmImf/ImfTiledInputPart.h                 |    30 +-
    OpenEXR/IlmImf/ImfTiledOutputFile.cpp              |    66 +-
    OpenEXR/IlmImf/ImfTiledOutputFile.h                |    39 +-
    OpenEXR/IlmImf/ImfTiledOutputPart.h                |    33 +-
    OpenEXR/IlmImf/ImfTiledRgbaFile.h                  |    83 +-
    OpenEXR/IlmImf/ImfTimeCode.h                       |    35 +-
    OpenEXR/IlmImf/ImfVersion.h                        |     4 +-
    OpenEXR/IlmImf/ImfZip.cpp                          |   191 +-
    OpenEXR/IlmImf/ImfZip.h                            |     8 +
    OpenEXR/IlmImf/ImfZipCompressor.h                  |     5 +
    OpenEXR/IlmImf/Makefile.am                         |    12 +-
    OpenEXR/IlmImf/dwaLookups.cpp                      |    10 +-
    OpenEXR/IlmImfExamples/CMakeLists.txt              |    18 +-
    OpenEXR/IlmImfExamples/Makefile.am                 |     8 +-
    OpenEXR/IlmImfExamples/previewImageExamples.cpp    |     6 +-
    OpenEXR/IlmImfFuzzTest/CMakeLists.txt              |    27 +-
    OpenEXR/IlmImfFuzzTest/Makefile.am                 |     6 +-
    OpenEXR/IlmImfTest/CMakeLists.txt                  |    18 +-
    OpenEXR/IlmImfTest/Makefile.am                     |     6 +-
    OpenEXR/IlmImfTest/compareDwa.h                    |     4 +-
    OpenEXR/IlmImfTest/testDwaCompressorSimd.cpp       |    47 +-
    OpenEXR/IlmImfUtil/CMakeLists.txt                  |   113 +-
    OpenEXR/IlmImfUtil/ImfDeepImage.h                  |    33 +-
    OpenEXR/IlmImfUtil/ImfDeepImageChannel.h           |    35 +-
    OpenEXR/IlmImfUtil/ImfDeepImageIO.h                |    26 +-
    OpenEXR/IlmImfUtil/ImfDeepImageLevel.cpp           |     2 +-
    OpenEXR/IlmImfUtil/ImfDeepImageLevel.h             |    44 +-
    OpenEXR/IlmImfUtil/ImfFlatImage.h                  |    29 +-
    OpenEXR/IlmImfUtil/ImfFlatImageChannel.h           |    10 +-
    OpenEXR/IlmImfUtil/ImfFlatImageIO.h                |    26 +-
    OpenEXR/IlmImfUtil/ImfFlatImageLevel.cpp           |     2 +-
    OpenEXR/IlmImfUtil/ImfFlatImageLevel.h             |    31 +-
    OpenEXR/IlmImfUtil/ImfImage.cpp                    |     4 +-
    OpenEXR/IlmImfUtil/ImfImage.h                      |    31 +-
    OpenEXR/IlmImfUtil/ImfImageChannel.h               |    10 +-
    OpenEXR/IlmImfUtil/ImfImageDataWindow.cpp          |     3 +-
    OpenEXR/IlmImfUtil/ImfImageDataWindow.h            |     2 +
    OpenEXR/IlmImfUtil/ImfImageIO.h                    |    10 +-
    OpenEXR/IlmImfUtil/ImfImageLevel.cpp               |     2 +-
    OpenEXR/IlmImfUtil/ImfImageLevel.h                 |    20 +-
    OpenEXR/IlmImfUtil/ImfSampleCountChannel.h         |    23 +-
    OpenEXR/IlmImfUtil/ImfUtilExport.h                 |    46 +
    OpenEXR/IlmImfUtil/Makefile.am                     |    16 +-
    OpenEXR/IlmImfUtilTest/CMakeLists.txt              |    20 +-
    OpenEXR/IlmImfUtilTest/Makefile.am                 |     6 +-
    OpenEXR/Makefile.am                                |     5 +-
    OpenEXR/README                                     |    77 -
    OpenEXR/README.CVS                                 |    16 -
    OpenEXR/README.OSX                                 |    57 -
    OpenEXR/README.cmake.txt                           |    54 -
    OpenEXR/README.git                                 |    16 -
    OpenEXR/README.md                                  |   132 +
    OpenEXR/README.namespacing                         |    83 -
    OpenEXR/bootstrap                                  |     4 +-
    OpenEXR/build.log                                  | 11993 -------------------
    OpenEXR/configure.ac                               |   284 +-
    OpenEXR/doc/Makefile.am                            |     1 -
    OpenEXR/doc/TheoryDeepPixels.pdf                   |   Bin 331719 -> 334777 bytes
    OpenEXR/exr2aces/CMakeLists.txt                    |    10 +-
    OpenEXR/exrbuild/CMakeLists.txt                    |    13 +-
    OpenEXR/exrenvmap/CMakeLists.txt                   |    10 +-
    OpenEXR/exrenvmap/Makefile.am                      |     6 +-
    OpenEXR/exrheader/CMakeLists.txt                   |    15 +-
    OpenEXR/exrheader/Makefile.am                      |     6 +-
    OpenEXR/exrmakepreview/CMakeLists.txt              |    10 +-
    OpenEXR/exrmakepreview/Makefile.am                 |     6 +-
    OpenEXR/exrmakepreview/makePreview.cpp             |     6 +-
    OpenEXR/exrmaketiled/CMakeLists.txt                |     9 +-
    OpenEXR/exrmaketiled/Makefile.am                   |     6 +-
    OpenEXR/exrmaketiled/makeTiled.cpp                 |     8 +-
    OpenEXR/exrmultipart/CMakeLists.txt                |    13 +-
    OpenEXR/exrmultipart/Makefile.am                   |     8 +-
    OpenEXR/exrmultiview/CMakeLists.txt                |    12 +-
    OpenEXR/exrmultiview/Makefile.am                   |     6 +-
    OpenEXR/exrstdattr/CMakeLists.txt                  |    13 +-
    OpenEXR/exrstdattr/Makefile.am                     |     6 +-
    OpenEXR/m4/ax_cxx_compile_stdcxx.m4                |   982 ++
    OpenEXR/m4/path.pkgconfig.m4                       |    63 +-
    OpenEXR_Viewers/AUTHORS                            |    12 -
    OpenEXR_Viewers/CMakeLists.txt                     |    71 +-
    OpenEXR_Viewers/COPYING                            |    34 -
    OpenEXR_Viewers/INSTALL                            |     2 -
    OpenEXR_Viewers/Makefile.am                        |     6 +-
    OpenEXR_Viewers/NEWS                               |     2 -
    OpenEXR_Viewers/README                             |    95 -
    OpenEXR_Viewers/README.CVS                         |    16 -
    OpenEXR_Viewers/README.OSX                         |    18 -
    OpenEXR_Viewers/README.md                          |   278 +
    OpenEXR_Viewers/README.win32                       |   196 -
    OpenEXR_Viewers/bootstrap                          |     4 +-
    OpenEXR_Viewers/configure.ac                       |    47 +-
    OpenEXR_Viewers/exrdisplay/CMakeLists.txt          |    15 +-
    OpenEXR_Viewers/exrdisplay/GlWindow3d.h            |     5 +
    OpenEXR_Viewers/m4/ax_cxx_compile_stdcxx.m4        |   982 ++
    OpenEXR_Viewers/playexr/CMakeLists.txt             |     8 +-
    PyIlmBase/AUTHORS                                  |    10 -
    PyIlmBase/CMakeLists.txt                           |   128 +-
    PyIlmBase/COPYING                                  |    34 -
    PyIlmBase/INSTALL                                  |     2 -
    PyIlmBase/Makefile.am                              |     7 +-
    PyIlmBase/NEWS                                     |     2 -
    PyIlmBase/PyIex/CMakeLists.txt                     |    52 +-
    PyIlmBase/PyIex/PyIex.cpp                          |     4 +-
    PyIlmBase/PyIex/PyIex.h                            |     4 +-
    PyIlmBase/PyIex/PyIexExport.h                      |    45 +-
    PyIlmBase/PyIex/iexmodule.cpp                      |     5 +-
    PyIlmBase/PyIexTest/CMakeLists.txt                 |     4 +-
    PyIlmBase/PyImath/CMakeLists.txt                   |    53 +-
    PyIlmBase/PyImath/PyImath.cpp                      |     5 +-
    PyIlmBase/PyImath/PyImath.h                        |     8 +-
    PyIlmBase/PyImath/PyImathAutovectorize.cpp         |     2 +-
    PyIlmBase/PyImath/PyImathAutovectorize.h           |     6 +-
    PyIlmBase/PyImath/PyImathBasicTypes.cpp            |     9 +-
    PyIlmBase/PyImath/PyImathBasicTypes.h              |     4 +-
    PyIlmBase/PyImath/PyImathBox.cpp                   |    18 +-
    PyIlmBase/PyImath/PyImathBox.h                     |     4 +-
    PyIlmBase/PyImath/PyImathBox2Array.cpp             |     4 +-
    PyIlmBase/PyImath/PyImathBox3Array.cpp             |     4 +-
    PyIlmBase/PyImath/PyImathBoxArrayImpl.h            |    10 +-
    PyIlmBase/PyImath/PyImathColor.h                   |     3 +-
    PyIlmBase/PyImath/PyImathColor3.cpp                |     8 +-
    PyIlmBase/PyImath/PyImathColor3ArrayImpl.h         |     4 +-
    PyIlmBase/PyImath/PyImathColor4.cpp                |     6 +-
    PyIlmBase/PyImath/PyImathColor4Array2DImpl.h       |     7 +-
    PyIlmBase/PyImath/PyImathColor4ArrayImpl.h         |     4 +-
    PyIlmBase/PyImath/PyImathEuler.cpp                 |     8 +-
    PyIlmBase/PyImath/PyImathEuler.h                   |     3 +-
    PyIlmBase/PyImath/PyImathExport.h                  |    52 +-
    PyIlmBase/PyImath/PyImathFixedArray.cpp            |     2 +-
    PyIlmBase/PyImath/PyImathFixedArray.h              |    11 +-
    PyIlmBase/PyImath/PyImathFixedArray2D.h            |     9 +
    PyIlmBase/PyImath/PyImathFixedMatrix.h             |     9 +
    PyIlmBase/PyImath/PyImathFixedVArray.cpp           |    14 +-
    PyIlmBase/PyImath/PyImathFixedVArray.h             |     2 +-
    PyIlmBase/PyImath/PyImathFrustum.cpp               |     8 +-
    PyIlmBase/PyImath/PyImathFrustum.h                 |     3 +-
    PyIlmBase/PyImath/PyImathFun.cpp                   |     8 +-
    PyIlmBase/PyImath/PyImathFun.h                     |     2 +-
    PyIlmBase/PyImath/PyImathLine.cpp                  |    16 +-
    PyIlmBase/PyImath/PyImathLine.h                    |     2 +-
    PyIlmBase/PyImath/PyImathM44Array.cpp              |     6 +-
    PyIlmBase/PyImath/PyImathM44Array.h                |     2 +-
    PyIlmBase/PyImath/PyImathMatrix.h                  |     3 +-
    PyIlmBase/PyImath/PyImathMatrix33.cpp              |     8 +-
    PyIlmBase/PyImath/PyImathMatrix44.cpp              |    10 +-
    PyIlmBase/PyImath/PyImathOperators.h               |     4 +-
    PyIlmBase/PyImath/PyImathPlane.cpp                 |    20 +-
    PyIlmBase/PyImath/PyImathPlane.h                   |     2 +-
    PyIlmBase/PyImath/PyImathQuat.cpp                  |    10 +-
    PyIlmBase/PyImath/PyImathQuat.h                    |     3 +-
    PyIlmBase/PyImath/PyImathRandom.cpp                |    10 +-
    PyIlmBase/PyImath/PyImathShear.cpp                 |     8 +-
    PyIlmBase/PyImath/PyImathStringArray.cpp           |     6 +-
    PyIlmBase/PyImath/PyImathStringArray.h             |     4 +-
    PyIlmBase/PyImath/PyImathStringArrayRegister.h     |     2 +-
    PyIlmBase/PyImath/PyImathStringTable.cpp           |     4 +-
    PyIlmBase/PyImath/PyImathTask.cpp                  |    10 +-
    PyIlmBase/PyImath/PyImathTask.h                    |    34 +-
    PyIlmBase/PyImath/PyImathUtil.cpp                  |     6 +-
    PyIlmBase/PyImath/PyImathUtil.h                    |    14 +-
    PyIlmBase/PyImath/PyImathVec.h                     |     4 +-
    PyIlmBase/PyImath/PyImathVec2Impl.h                |    12 +-
    PyIlmBase/PyImath/PyImathVec3ArrayImpl.h           |    12 +-
    PyIlmBase/PyImath/PyImathVec3Impl.h                |     6 +-
    PyIlmBase/PyImath/PyImathVec4ArrayImpl.h           |    10 +-
    PyIlmBase/PyImath/PyImathVec4Impl.h                |     6 +-
    PyIlmBase/PyImath/imathmodule.cpp                  |    38 +-
    PyIlmBase/PyImathNumpy/CMakeLists.txt              |    25 +-
    PyIlmBase/PyImathNumpy/imathnumpymodule.cpp        |    14 +-
    PyIlmBase/PyImathNumpyTest/CMakeLists.txt          |     6 +-
    PyIlmBase/PyImathNumpyTest/pyImathNumpyTest.in     |    81 +-
    PyIlmBase/PyImathTest/CMakeLists.txt               |     2 +
    PyIlmBase/PyImathTest/pyImathTest.in               |  1090 +-
    PyIlmBase/README                                   |    51 -
    PyIlmBase/README.OSX                               |    21 -
    PyIlmBase/README.md                                |    99 +
    PyIlmBase/bootstrap                                |     4 +-
    PyIlmBase/configure.ac                             |    64 +-
    PyIlmBase/m4/ax_cxx_compile_stdcxx.m4              |   982 ++
    README                                             |    68 -
    README.md                                          |   202 +
    cmake/FindIlmBase.cmake                            |   192 +
    cmake/FindNumPy.cmake                              |    51 +
    cmake/FindOpenEXR.cmake                            |   198 +
    321 files changed, 12796 insertions(+), 16398 deletions(-)
   
### Commits \[ git log v2.2.1...v.2.3.0\]

*  [Reverted python library -l line logic to go back to the old PYTHON_VERSION based logic.](https://github.com/openexr/openexr/commit/02310c624547fd765cd6e08abe459755d4ecebcc) ([Nick Rasmussen](@nick@ilm.com), 2018-08-09) 

*  [Updated build system to use local copies of the ax_cxx_copmile_stdcxx.m4 macro.](https://github.com/openexr/openexr/commit/3d6c9302b3d7f394a90ac3c95d12b1db1c183812) ([Nick Rasmussen](@nick@ilm.com), 2018-08-09) 

*  [accidentally commited Makefile instead of Makefile.am](https://github.com/openexr/openexr/commit/46dda162ef2b3defceaa25e6bdd2b71b98844685) ([Cary Phillips](@cary@ilm.com), 2018-08-09) 

*  [update CHANGES.md](https://github.com/openexr/openexr/commit/ea46c15be9572f81549eaa76a1bdf8dbe364f780) ([Cary Phillips](@cary@ilm.com), 2018-08-08) 

*  [Added FindNumPy.cmake](https://github.com/openexr/openexr/commit/63870bb10415ca7ea76ecfdafdfe70f5894f66f2) ([Nick Porcino](@meshula@hotmail.com), 2018-08-08) 

*  [Add PyImathNumpyTest to Makefile and configure.ac](https://github.com/openexr/openexr/commit/36abd2b728e8759b010ceffe94363d5f473fe6dc) ([Cary Phillips](@cary@ilm.com), 2018-08-08) 

*  [Add ImfExportUtil.h to Makefile.am](https://github.com/openexr/openexr/commit/82f78f4a895e29b42d2ccc0d66be08948203f507) ([Cary Phillips](@cary@ilm.com), 2018-08-08) 

*  [fix pyilmbase tests, static compilation](https://github.com/openexr/openexr/commit/75c918b65c2394c7f7a9f769fee87572d06e81b5) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-09) - python extensions must be shared, so can not follow the overall lib type for the library. - the code should be compiled fPIC when building a static library such that it can be linked into a .so - remove the dependency on the particle python extension in the numpy test - add environment variables such that the python tests will work in the build tree without a "make install" (win32 doesn't neede ld_library_path, but it doesn't hurt, but may need path?) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix OPENEXR_VERSION and OPENEXR_SOVERSION](https://github.com/openexr/openexr/commit/4481442b467e492a3a515b0992391dc160282786) ([Cary Phillips](@cary@ilm.com), 2018-08-08) 

*  [update readme documentation for new cmake option](https://github.com/openexr/openexr/commit/081c9f9f9f26afc6943f1b2e63d171802895bee5) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix compile errors under c++17](https://github.com/openexr/openexr/commit/6d9e3f6e2a9545e9d060f599967868d228d9a56a) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) Fixes errors with collisions due to the addition of clamp to the std namespace Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [add last ditch effort for numpy](https://github.com/openexr/openexr/commit/af5fa2d84acf74e411d6592201890b1e489978c4) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) Apparently not all distributions include a FindNumPy.cmake or similar, even if numpy is indeed installed. This makes a second effort to find using python itself Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [make pyilmbase tests conditional](https://github.com/openexr/openexr/commit/07951c8bdf6164e34f37c3d88799e4e98e46d1ee) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) This makes the PyIlmBase tests conditional in the same manner as OpenEXR and IlmBase Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [optimize regeneration of config files](https://github.com/openexr/openexr/commit/b610ff33e827c38ac3693d3e43ad973c891d808c) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) This makes the config files write to a temporary file, then use cmake's configure_file command with copyonly to compare the contents and not copy if they are the same. Incremental builds are much faster as a result when working on new features and adding files to the cmakelists.txt Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [make fuzz test optional like autoconf](https://github.com/openexr/openexr/commit/79a50ea7eb869a94bb226841aebad9d46ecc3836) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) This makes running the fuzz tests as part of the "make test" rule optional. Even with this off by default, if building tests is enabled, the fuzz test will still be compiled, and is available to run via "make fuzz". This should enable a weekly jenkins build config to run the fuzz tests, given that it takes a long time to run. Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [Fix SO version](https://github.com/openexr/openexr/commit/f4055c33bb128bd4544d265b167337c584364716) ([Nick Porcino](@meshula@hotmail.com), 2018-08-07) 

*  [CHANGES.md formatting](https://github.com/openexr/openexr/commit/8cd1b9210855fa4f6923c1b94df8a86166be19b1) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [format old release notes](https://github.com/openexr/openexr/commit/3c5b5f894def68cf5240e8f427147c867f745912) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [release notes upates](https://github.com/openexr/openexr/commit/534e4bcde71ce34b9f8fa9fc39e9df1a58aa3f80) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [CHANGES.md](https://github.com/openexr/openexr/commit/471d7bd1c558c54ecc3cbbb2a65932f1e448a370) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [OpenEXR_Viewers/README.md formatting](https://github.com/openexr/openexr/commit/806db743cf0bcb7710d08f56ee6f2ece10e31367) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [more README fixes.](https://github.com/openexr/openexr/commit/82bc701e605e092ae5f31d142450d921c293ded1) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [README.md cleanup](https://github.com/openexr/openexr/commit/d1d9760b084f460cf21de2b8e273e8d6adcfb4f6) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [fix dependencies when building static](https://github.com/openexr/openexr/commit/03329c8d34c93ecafb4a35a8cc645cd3bea14217) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-08) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix exrdisplay compile under cmake](https://github.com/openexr/openexr/commit/a617dc1a9cc8c7b85df040f5587f1727dec31caf) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-07) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [PyIlmBase README.md cleanup](https://github.com/openexr/openexr/commit/a385fd4f09ab5dd1163fab6870393f1b71e163eb) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [Updates to README's](https://github.com/openexr/openexr/commit/0690e762bb45afadd89e94838270080447998a48) ([Cary Phillips](@cary@ilm.com), 2018-08-07) 

*  [added --foreign to automake in bootstrap](https://github.com/openexr/openexr/commit/4a74696f2066dd4bb58433bbcb706fdf526a7770) ([Cary Phillips](@cary@ilm.com), 2018-08-06) 

*  [Remove obsolete README files from Makefile.am](https://github.com/openexr/openexr/commit/57259b7811f3adce23a1e4c99411d686c55fefed) ([Cary Phillips](@cary@ilm.com), 2018-08-06) 

*  [Removed COPYING, INSTALL, README.cmake.txt](https://github.com/openexr/openexr/commit/54d3bbcfef10a367591cced99f759b89e8478b07) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [cleaned up README files for root and IlmBase](https://github.com/openexr/openexr/commit/54e6ae149addd5b9673d1ee0f2954759b5ed073d) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [LIBTOOL_CURRENT=24](https://github.com/openexr/openexr/commit/7b7ea9c86bbf8744cb41df6fa7e5f7dd270294a5) ([Cary Phillips](@cary@ilm.com), 2018-08-06) 

*  [bump version to 2.3](https://github.com/openexr/openexr/commit/8a7b4ad263103e725fda4e624962cc0f559c4faa) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [folding in internal ILM changes - conditional delete in exception catch block.](https://github.com/openexr/openexr/commit/656f898dff3ab7d06c4d35219385251f7948437b) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [Removed COPYING, INSTALL, README.cmake.txt](https://github.com/openexr/openexr/commit/94ece7ca86ffccb3ec2bf4138f4ad47e3f496167) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [edits to READMEs](https://github.com/openexr/openexr/commit/405fa911ad974eeaf3c3769820b7c4a0c59f0099) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [README fixes.](https://github.com/openexr/openexr/commit/c612d8276a5d9e28ae6bdc39b770cbc083e21cf4) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [cleaned up README files for root and IlmBase](https://github.com/openexr/openexr/commit/cda04c6451b0b196c887b03e68d8a80863f58832) ([Cary Phillips](@cary@ilm.com), 2018-08-05) 

*  [Fallback default system provided Boost Python](https://github.com/openexr/openexr/commit/a174497d1fd84378423f733053f1a058608d81f0) ([Thanh Ha](@thanh.ha@linuxfoundation.org), 2018-08-03) User provided Python version via OPENEXR_PYTHON_MAJOR and OPENEXR_PYTHON_MINOR parameters, failing that fallback onto the system's default "python" whichever that may be. Signed-off-by: Thanh Ha <thanh.ha@linuxfoundation.org> 

*  [fix double delete in multipart files, check logic in others](https://github.com/openexr/openexr/commit/da96e3759758c1fcac5963e07eab8e1f58a674e7) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) Multipart files have a Data object that automatically cleans up it's stream if appropriate, the other file objects have the destructor of the file object perform the delete (instead of Data). This causes a double delete to happen in MultiPart objects when unable to open a stream. Additionally, fix tabs / spaces to just be spaces Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix scenario where ilmimf is being compiled from parent directory](https://github.com/openexr/openexr/commit/c246315fe392815399aee224f38bafd01585594b) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) need to use current source dir so test images can be found Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [Fix logic errors with BUILD_DWALOOKUPS](https://github.com/openexr/openexr/commit/dc7cb41c4e8a3abd60dec46d0bcb6a1c9ef31452) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [remove debug print](https://github.com/openexr/openexr/commit/8e16aa8930a85f1ef3f1f6ba454af275aabc205d) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fully set variables as pkg config does not seem to by default](https://github.com/openexr/openexr/commit/f478511f796e5d05dada28f9841dcf9ebd9730ac) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) 

*  [add check with error message for zlib, fix defaults, restore old thread check](https://github.com/openexr/openexr/commit/788956537282cfcca712c1e9690d72cd19978ce0) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-04) 

*  [PR #187 CMake enhancements to speed up dependency builds of OpenEXR.](https://github.com/openexr/openexr/commit/17e10ab10ddf937bc2809bda858bf17af6fb3448) ([Nick Porcino](@meshula@hotmail.com), 2018-08-02) 

*  [restore prefix, update to use PKG_CHECK_MODULES](https://github.com/openexr/openexr/commit/fb9d1be5c07779c90e7744ccbf27201fcafcdfdb) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-03) previous commit from dracwyrm had made it such that pkg-config must be used and ilmbase must be installed in the default pkg-config path by default. restore the original behaviour by which a prefix could be provided, yet still retain use of PKG_CHECK_MODULES to find IlmBase if the prefix is not specified, and continue to use pkg-config to find zlib instead of assuming -lz Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [restore original API for Lock since we can't use typedef to unique_lock](https://github.com/openexr/openexr/commit/e7fc2258a16ab7fe17d24855d16d4e56b80c172e) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-02) 

*  [fixes #292, issue with utf-8 filenames](https://github.com/openexr/openexr/commit/846fe64c584ebb89434aaa02f5d431fbd3ca6165) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-01) windows needs to widen the string to properly open files, this implements a solution for compiling with MSVC anyway using the extension for fstream to take a wchar Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix maintainer mode issue, extra line in paste](https://github.com/openexr/openexr/commit/772ff9ad045032fc338af1b684cb50983191bc0d) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-08-02) 

*  [Default the python bindings to on](https://github.com/openexr/openexr/commit/dc5e26136b1c5edce911ff0eccc17cda40388b54) ([Nick Porcino](@meshula@hotmail.com), 2018-08-01) 

*  [Add Find scripts, and ability to build OpenEXR with pre-existing IlmBase](https://github.com/openexr/openexr/commit/34ee51e9118097f784653f08c9482c886f83d2ef) ([Nick Porcino](@meshula@hotmail.com), 2018-08-01) 

*  [fix names, disable rules when not building shared](https://github.com/openexr/openexr/commit/dbd3b34baf4104e844c273b682e7b133304294f2) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-31) 

*  [add suffix variable for target names to enable static-only build](https://github.com/openexr/openexr/commit/7b1ed10e241e793db9d8933df30dd305a93835dd) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-31) 

*  [The string field is now called _message.](https://github.com/openexr/openexr/commit/bd32e84632da4754cfe6db47f2e72c29f4d7df27) ([Cary Phillips](@cary@ilm.com), 2018-08-01) 

*  [C++11 support for numeric_limits<Half>::max_digits10() and lowest()](https://github.com/openexr/openexr/commit/2d931bab38840ab3cdf9c6322767a862aae4037d) ([Cary Phillips](@cary@ilm.com), 2018-07-31) 

*  [fixes for GCC 6.3.1 (courtesy of Will Harrower): - renamed local variables in THROW macros to avoid warnings - cast to bool](https://github.com/openexr/openexr/commit/7fda69a377ee41979284137795cb338bb3c6d147) ([Cary Phillips](@cary@rnd-build7-sf-38.lucasfilm.com), 2018-07-31) 

*  [renames name to message and removes implicit cast](https://github.com/openexr/openexr/commit/54105e3c292c6884e7870ecfddb561deda7a3458) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-31) This removes the implicit cast, which is arguably more "standard", and also less surprising. Further, renames the name function to message to match internal ILM changes, and message makes more sense as a function name than ... name. 

*  [Remove IEX_THROW_SPEC](https://github.com/openexr/openexr/commit/02c896501da244ec6345d7ee5ef825d71ba1f0a2) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-31) This removes the macro and uses therein. We changed the API with removing the subclass from std::string of Iex::BaseExc, so there is no reason to retain this compatibility as well, especially since it isn't really meaningful anyway in (modern) C++ 

*  [CMake3 port. Various Windows fixes](https://github.com/openexr/openexr/commit/b2d37be8b874b300be1907f10339cac47e39170b) ([Nick Porcino](@meshula@hotmail.com), 2018-07-29) 

*  [changes to enable custom namespace defines to be used](https://github.com/openexr/openexr/commit/acd76e16276b54186096b04b06bd118eb32a1bcf) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-29) 

*  [fix extraneous unsigned compare accidentally merged](https://github.com/openexr/openexr/commit/a56773bd7a1f9a8bb10afe5fb36c4e03f622eff6) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-29) 

*  [Use proper definition of namespaces instead of default values.](https://github.com/openexr/openexr/commit/c6978f9fd998df32b2c56a7b25bbbd52005bbf9e) ([Juri Abramov](@gabramov@nvidia.com), 2014-08-18) 

*  [fixes #260, out of bounds vector access](https://github.com/openexr/openexr/commit/efc360fc17935453e95f62939dd5d7caacce4bf7) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-29) noticed by Google Autofuzz Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [fix potential io streams leak in case of exceptions during 'initialize' function](https://github.com/openexr/openexr/commit/19bac86f27bab8649858ef79658224e9a54cb4cf) ([CAHEK7](@ghosts.in.a.box@gmail.com), 2016-02-12) 

*  [OpenEXR: Fix build system and change doc install](https://github.com/openexr/openexr/commit/60cc8b711ab402c5526ca1f872de5209ad15ec7d) ([dracwyrm](@j.scruggs@gmail.com), 2017-08-11) The build sysem for the OpenEXR sub-module is has issues. This patch is being used on Gentoo Linux with great success. It also adresses the issue of linking to previously installed versions. Signed-off by: Jonathan Scruggs (j.scruggs@gmail.com) Signed-off by: David Seifert (soap@gentoo.org) 

*  [Note that numpy is required to build PyIlmBase](https://github.com/openexr/openexr/commit/76935a912a8e365ed4fe8c7a54b60561790dafd5) ([Thanh Ha](@thanh.ha@linuxfoundation.org), 2018-07-20) Signed-off-by: Thanh Ha <thanh.ha@linuxfoundation.org> 

*  [Fixed exports on DeepImageChannel and FlatImageChannel. If the whole class isn't exported, the typeinfo doesn't get exported, and so dynamic casting into those classes will not work.](https://github.com/openexr/openexr/commit/942ff971d30cba1b237c91e9f448376d279dc5ee) ([Halfdan Ingvarsson](@halfdan@sidefx.com), 2014-10-06) Also fixed angle-bracket include to a quoted include. 

*  [Fixed angle bracket vs quote includes.](https://github.com/openexr/openexr/commit/fd8570927a7124ff2990f5f38556b2ec03d77a44) ([Halfdan Ingvarsson](@halfdan@sidefx.com), 2014-03-18) 

*  [Change IexBaseExc to no longer derive from std::string, but instead include it as a member variable. This resolves a problem with MSVC 2012 and dllexport-ing template classes.](https://github.com/openexr/openexr/commit/fa59776fd83a8f35ed5418b83bbc9975ba0ef3bc) ([Halfdan Ingvarsson](@halfdan@sidefx.com), 2014-03-03) 

*  [make code more amenable to compiling with mingw for cross-compiling](https://github.com/openexr/openexr/commit/dd867668c4c63d23c034cc2ea8f2352451e8554d) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-07-29) Signed-off-by: Kimball Thurston <kdt3rd@gmail.com> 

*  [Fix shebang line to bash](https://github.com/openexr/openexr/commit/d3512e07a5af5053397ed62bd0d306b10357358c) ([Thanh Ha](@thanh.ha@linuxfoundation.org), 2018-07-19) Depending on the distro running the script the following error might appear: ./bootstrap: 4: [: Linux: unexpected operator This is because #!/bin/sh is not the same on every distro and this script is actually expecting bash. So update the shebang line to be bash. Signed-off-by: Thanh Ha <thanh.ha@linuxfoundation.org> 

*  [Visual Studio and Windows fixes](https://github.com/openexr/openexr/commit/4cfefeab4be94b8c46d604075367b6496d29dcb5) ([Liam Fernandez](@liam@utexas.edu), 2018-06-20) IlmBase: Fix IF/ELSEIF clause (WIN32 only) PyImath: Install *.h in 'include' dir PyImathNumpy: Change python library filename to 'imathnumpy.pyd' (WIN32 only) 

*  [Fix probable typo for static builds.](https://github.com/openexr/openexr/commit/31e1ae8acad3126a63044dfb8518d70390131c7b) ([Simon Otter](@skurmedel@gmail.com), 2018-06-18) 

*  [Must also export protected methods](https://github.com/openexr/openexr/commit/17384ee01e5fa842f282c833ab2bc2aa33e07125) ([Nick Porcino](@meshula@hotmail.com), 2018-06-10) 

*  [IlmImfUtilTest compiles successfully](https://github.com/openexr/openexr/commit/6093789bc7b7c543f128ab2b055987808ec15167) ([Nick Porcino](@meshula@hotmail.com), 2018-06-09) 

*  [IlmImfUtil now builds on Windows](https://github.com/openexr/openexr/commit/d7328287d1ea363ab7839201e90d7d7f4deb635f) ([Nick Porcino](@meshula@hotmail.com), 2018-06-09) 

*  [Set python module suffix per platform](https://github.com/openexr/openexr/commit/39b9edfdfcad5e77601d4462a6f9ba93bef83835) ([Nick Porcino](@meshula@hotmail.com), 2018-06-05) 

*  [fix include ifdef](https://github.com/openexr/openexr/commit/32723d8112d1addf0064e8295b824faab60f0162) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-05-26) 

*  [switch from shared pointer to a manually counted object as gcc 4.8 and 4.9 do not provide proper shared_ptr atomic functions](https://github.com/openexr/openexr/commit/3f532a7ab81c33f61dc6786a8c7ce6e0c09acc07) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-05-26) 

*  [Fix typos to TheoryDeepPixels document](https://github.com/openexr/openexr/commit/655f96032e0eddd868a122fee80bd558e0cbf17d) ([peterhillman](@peter@peterhillman.org.uk), 2018-05-17) Equations 6 and 7 were incorrect. 

*  [initial port of PyIlmBase to python 3](https://github.com/openexr/openexr/commit/84dbf637c5c3ac4296181dd93de4fb5ffdc4b582) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-05-04) 

*  [replicate configure / cmake changes from ilmbase](https://github.com/openexr/openexr/commit/00df2c72ca1b7cb148e19a9bdc44651a6c74c9e4) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-05-04) This propagates the same chnages to configure.ac and cmakelists.txt to enable compiling with c++11/14. Additionally, adds some minor changes to configure to enable python 3 to be configured (source code changes tbd) 

*  [add move constructor and assignment operator](https://github.com/openexr/openexr/commit/cfebcc24e1a1cc307678ea757ec38bff02a5dc51) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-05-03) 

*  [Fix Windows Python binding builds. Does not address PyImath runtime issues, but does allow build to succeed](https://github.com/openexr/openexr/commit/15ce54ca02fdfa16c4a99f45a30c7a54826c6ac3) ([Nick Porcino](@meshula@hotmail.com), 2018-04-30) 

*  [Fix c++11 detection issue on windows. Fix ilmbase DLL export warnings](https://github.com/openexr/openexr/commit/7376f9b736f9503a9d34b67c99bc48ce826a6334) ([Nick Porcino](@meshula@hotmail.com), 2018-04-27) 

*  [enable different c++ standards to be selected instead of just c++14](https://github.com/openexr/openexr/commit/99ecfcabbc2b95acb40283f04ab358b3db9cc0f9) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-15) 

*  [Incorporate review feedback](https://github.com/openexr/openexr/commit/99b367d963ba0892e7ab830458b6a990aa3033ce) ([Nick Porcino](@meshula@hotmail.com), 2018-04-04) 

*  [add compatibility std::condition_variable semaphore when posix semaphores not available](https://github.com/openexr/openexr/commit/b6dc2a6b71f9373640d988979f9ae1929640397a) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [fix error overwriting beginning of config file](https://github.com/openexr/openexr/commit/01680dc4d90c9f7fd64e498e57588f630a52a214) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [remove the dynamic exception for all versions of c++ unless FORCE_CXX03 is on](https://github.com/openexr/openexr/commit/45cb2c8fb2418afaa3900c553e26ad3886cd5acf) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [ThreadPool improvements](https://github.com/openexr/openexr/commit/bf0cb8cdce32fce36017107c9982e1e5db2fb3fa) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) - switch to use c++11 features - Add API to enable replacement of the thread pool - Add custom, low-latency handling when threads is 0 - Lower lock boundary when adding tasks (or eliminate in c++11 mode) 

*  [switch mutex to be based on std::mutex when available](https://github.com/openexr/openexr/commit/848c8c329b16aeee0d3773e827d506a2a53d4840) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [switch IlmThread to use c++11 threads when enabled](https://github.com/openexr/openexr/commit/eea1e607177e339e05daa1a2ec969a9dd12f2497) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [use dynamic exception macro to avoid warnings in c++14 mode](https://github.com/openexr/openexr/commit/610179cbe3ffc2db206252343e75a16221d162b4) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [add #define to manage dynamic exception deprecation in c++11/14](https://github.com/openexr/openexr/commit/b133b769aaee98566e695191476f59f32eece591) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [configuration changes to enable c++14](https://github.com/openexr/openexr/commit/5f58c94aea83d44e27afd1f65e4defc0f523f6be) ([Kimball Thurston](@kdt3rd@gmail.com), 2018-04-04) 

*  [Cmake now building OpenEXR successfully for Windows](https://github.com/openexr/openexr/commit/ac055a9e50c974f4cd58c28a5a0bb96011812072) ([Nick Porcino](@meshula@hotmail.com), 2018-03-28) 

*  [Missing symbols on Windows due to missing IMF_EXPORT](https://github.com/openexr/openexr/commit/965c1eb6513ad80c71b425c8a1b04a70b3bae291) ([Ibraheem Alhashim](@ibraheem.alhashim@gmail.com), 2018-03-05) 

*  [Implement SIMD-accelerated ImfZip::uncompress](https://github.com/openexr/openexr/commit/32f2aa58fe4f6f6691eef322fdfbbc9aa8363f80) ([John Loy](@jloy@pixar.com), 2017-04-12) The main bottleneck in ImfZip::uncompress appears not to be zlib but the predictor & interleaving loops that run after zlib's decompression. Fortunately, throughput in both of these loops can be improved with SIMD operations. Even though each trip of the predictor loop has data dependencies on all previous values, the usual SIMD prefix-sum construction is able to provide a significant speedup. While the uses of SSSE3 and SSE4.1 are minor in this change and could maybe be replaced with some slightly more complicated SSE2, SSE4.1 was released in 2007, so it doesn't seem unreasonable to require it in 2017. 

*  [Compute sample locations directly in Imf::readPerDeepLineTable.](https://github.com/openexr/openexr/commit/e64095257a29f9bc423298ee8dbc09a317f22046) ([John Loy](@jloy@pixar.com), 2017-04-06) By changing the function to iterate over sample locations directly instead of discarding unsampled pixel positions, we can avoid computing a lot of modulos (more than one per pixel.) Even on modern x86 processors, idiv is a relatively expensive instruction. Though it may appear like this optimization could be performed by a sufficiently sophisticated compiler, gcc 4.8 does not get there (even at -O3.) 

*  [Manually hoist loop invariants in Imf::bytesPerDeepLineTable.](https://github.com/openexr/openexr/commit/71b8109a4ad123ef0d5783f01922463a16d2ca59) ([John Loy](@jloy@pixar.com), 2017-04-05) This is primarily done to avoid a call to pixelTypeSize within the inner loop. In particular, gcc makes the call to pixelTypeSize via PLT indirection so it may have arbitrary side-effects (i.e. ELF symbol interposition strikes again) and may not be moved out of the loop by the compiler. 

*  [Inline Imf::sampleCount; this is an ABI-breaking change.](https://github.com/openexr/openexr/commit/5aa0afd5a4f8df9e09d6461f115e6e0cec5cbe46) ([John Loy](@jloy@pixar.com), 2017-03-29) gcc generates calls to sampleCount via PLT indirection even within libIlmImf. As such, they are not inlined and must be treated as having arbitrary side effects (because of ELF symbol interposition.) Making addressing computations visible at call sites allows a much wider range of optimizations by the compiler beyond simply eliminating the function call overhead. 

*  [Delete build.log](https://github.com/openexr/openexr/commit/148f1c230b5ecd94d795ca172a8246785c7caca7) ([Arkady Shapkin](@arkady.shapkin@gmail.com), 2017-02-18) 

*  [fix defect in semaphore implementation which caused application hang at exit time, because not all worker threads get woken up when task semaphore is repeatedly posted (to wake them up) after setting the stopping flag in the thread pool](https://github.com/openexr/openexr/commit/4706d615e942462a532381a8a86bc5fe820c6816) ([Richard Goedeken](@Richard@fascinationsoftware.com), 2016-11-22) 

*  [fix comparison of unsigned expression < 0 (Issue #165)](https://github.com/openexr/openexr/commit/9e3913c94c55549640c732f549d2912fbd85c336) ([CAHEK7](@ghosts.in.a.box@gmail.com), 2016-02-15) 

*  [Added Iex library once more for linker dependency](https://github.com/openexr/openexr/commit/b0b50791b5b36fddb010b5ad630dd429f947a080) ([Eric Sommerlade](@es0m@users.noreply.github.com), 2015-02-20) 

*  [windows/cmake: Commands depend on Half.dll which needs to be in path. Running commands in Half.dll's directory addresses this and the commands run on first invocation](https://github.com/openexr/openexr/commit/1a23716fd7e9ae167f53c7f2099651ede1279fbb) ([E Sommerlade](@es0m@users.noreply.github.com), 2015-02-10) 

*  [Fixed memory corruption / actual crashes on Window](https://github.com/openexr/openexr/commit/c330c40e1962257b0e59328fdceaa9cdcde3041b) ([JuriAbramov](@openexr@dr-abramov.de), 2015-01-19) Fixed memory corruption caused by missing assignment operator with non-trivial copy constructor logic. FIxes crashes on Windows when "dwaa" or "dwab" codecs are used for saving files. 

*  [std namespace should be specified for transform](https://github.com/openexr/openexr/commit/4a00a9bc6c92b20443c61f5e9877123e7fef16e6) ([JuriAbramov](@openexr@dr-abramov.de), 2014-08-20) Fixes build with some VS and clang version. 

*  [m4/path.pkgconfig.m4: use PKG_PROG_PKG_CONFIG to find correct pkg-config](https://github.com/openexr/openexr/commit/056cb9f09efa9116c7f5fb8bc0717a260ad23744) ([Michael Thomas (malinka)](@malinka@entropy-development.com), 2016-05-24) pkg-config supplies this macro and prefers it to be used to allow for cross-compilation scenarios where target-prefixed binaries are prefered to pkg-config 

*  [Updated list of EXTRA_DIST files to reflect the updated test images and prior removal of README.win32](https://github.com/openexr/openexr/commit/165dceaeee86e0f8ce1ed1db3e3030c609a49f17) ([Nick Rasmussen](@nick@ilm.com), 2017-11-17) 

*  [Updated list of EXTRA_DIST files to reflect the updated test images and prior removal of README.win32](https://github.com/openexr/openexr/commit/dcaf5fdb4d1244d8e60a58832cfe9c54734a2257) ([Nick Rasmussen](@nick@ilm.com), 2017-11-17) 

*  [Updated openexr version to 2.2.1, resynced the .so version number to 23 across all projects.](https://github.com/openexr/openexr/commit/e69de40ddbb6bd58341618a506b2e913e5ac1797) ([Nick Rasmussen](@nick@ilm.com), 2017-11-17) 

*  [Add additional input validation in an attempt to resolve issue #232](https://github.com/openexr/openexr/commit/49db4a4192482eec9c27669f75db144cf5434804) ([Shawn Walker-Salas](@shawn.walker@oracle.com), 2017-05-30) 

*  [Add additional input validation in an attempt to resolve issue #232](https://github.com/openexr/openexr/commit/f09f5f26c1924c4f7e183428ca79c9881afaf53c) ([Shawn Walker-Salas](@shawn.walker@oracle.com), 2017-05-30) 

*  [root level LICENSE](https://github.com/openexr/openexr/commit/a774d643b566d56314f26695f2bf9b75f88e64f6) ([cary-ilm](@cary@ilm.com), 2017-10-23) 

*  [Fix copyright/license notice in halfExport.h](https://github.com/openexr/openexr/commit/20d043d017d4b752356bb76946ffdffaa9c15c72) ([Ed Hanway](@ehanway@ilm.com), 2017-01-09) 

*  [Merge branch 'jkingsman-cleanup-readme' into develop](https://github.com/openexr/openexr/commit/6f6d9cea513ea409d4b65da40ac096eab9a549b0) ([Ed Hanway](@ehanway@ilm.com), 2016-10-28) 

*  [README edits.](https://github.com/openexr/openexr/commit/098a4893910d522b867082ed38d7388e6265bee0) ([Ed Hanway](@ehanway@ilm.com), 2016-10-28) 

*  [Merge branch 'cleanup-readme' of https://github.com/jkingsman/openexr into jkingsman-cleanup-readme](https://github.com/openexr/openexr/commit/43e50ed5dca1ddfb3ca2cb4c38c7752497db6e50) ([Ed Hanway](@ehanway@ilm.com), 2016-10-28) 

*  [Install ImfStdIO.h](https://github.com/openexr/openexr/commit/2872d3b230a7920696510f80a50d9ce36b6cc94e) ([Ed Hanway](@ehanway@ilm.com), 2016-10-28) This was originally intended to be an internal class only, but its use has become the de facto way to handle UTF-8 filenames on Windows. 

*  [Merge pull request #204 from dlemstra/IMF_HAVE_SSE2](https://github.com/openexr/openexr/commit/cbb01bf286a2e04df95fb51458d1c2cbdc08935b) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-10-19) Consistent check for IMF_HAVE_SSE2. 

*  [Remove fixed-length line breaks](https://github.com/openexr/openexr/commit/0ea6b8c7d077a18fb849c2b2ff532cd952d06a38) ([Jack Kingsman](@jack.kingsman@gmail.com), 2016-10-19) 

*  [Update README to markdown](https://github.com/openexr/openexr/commit/9c6d22e23a25d761f5456e08623b8d77c0f8930a) ([Jack Kingsman](@jack.kingsman@gmail.com), 2016-10-18) 

*  [Merge pull request #206 from lgritz/lg-register](https://github.com/openexr/openexr/commit/6788745398594d479e8cf91a6c301fea0537108b) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-09-30) Remove 'register' keyword. 

*  [Remove 'register' keyword.](https://github.com/openexr/openexr/commit/6d297f35c5dbfacc8a5e94f33b986db7ab468db9) ([Larry Gritz](@lg@larrygritz.com), 2016-09-30) 'register' is a relic of K&R-era C, it's utterly useless in modern compilers. It's been deprecated in C++11, and therefore will generate warnings when encountered -- and many packages that use OpenEXR's public headers use -Werr to turn warnings into errors. Starting in C++17, the keyword is removed entirely, and thus will certainly be a build break for that version of the standard. So it's time for it to go. 

*  [Consistent check for IMF_HAVE_SSE2.](https://github.com/openexr/openexr/commit/7403524c8fed971383c724d85913b2d52672caf3) ([dirk](@dirk@git.imagemagick.org), 2016-09-17) 

*  [Merge pull request #141 from lucywilkes/develop](https://github.com/openexr/openexr/commit/c23f5345a6cc89627cc416b3e0e6b182cd427479) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-09-16) Adding rawPixelDataToBuffer() function for access to compressed scanlines 

*  [Merge pull request #198 from ZeroCrunch/develop](https://github.com/openexr/openexr/commit/891437f74805f6c8ebc897932091cbe0bb7e1163) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-08-02) Windows compile fix 

*  [Windows compile fix](https://github.com/openexr/openexr/commit/77faf005b50e8f77a8080676738ef9b9c807bf53) ([Jamie Kenyon](@jamie.kenyon@thefoundry.co.uk), 2016-07-29) std::min wasn't found due to <algorithm> not being included. 

*  [Merge pull request #179 from CAHEK7/NullptrBug](https://github.com/openexr/openexr/commit/a0a68393a4d3b622251fb7c490ee9d59e080b776) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-07-26) fix potential memory leak 

*  [Merge branch 'develop' of https://github.com/r-potter/openexr into r-potter-develop](https://github.com/openexr/openexr/commit/b206a243a03724650b04efcdf863c7761d5d5d5b) ([Ed Hanway](@ehanway@ilm.com), 2016-07-26) 

*  [Merge pull request #154 into develop](https://github.com/openexr/openexr/commit/bc372d47186db31d104e84e4eb9e84850819db8d) ([Ed Hanway](@ehanway@ilm.com), 2016-07-25) 

*  [Merge pull request #168 into develop](https://github.com/openexr/openexr/commit/44d077672f558bc63d907891bb88d741b334d807) ([Ed Hanway](@ehanway@ilm.com), 2016-07-25) 

*  [Merge pull request #175 into develop](https://github.com/openexr/openexr/commit/7513fd847cf38af89572cc209b03e5b548e6bfc8) ([Ed Hanway](@ehanway@ilm.com), 2016-07-25) 

*  [Merge pull request #174 into develop](https://github.com/openexr/openexr/commit/b16664a2ee4627c235b9ce798f4fc911e9c5694f) ([Ed Hanway](@ehanway@ilm.com), 2016-07-25) 

*  [Merge branch pull request 172 into develop: fix copy and paste bug in ImfDeepTiledInputPart.cpp](https://github.com/openexr/openexr/commit/ef7b78d5988d37dbbc74c21ad245ed5c80927223) ([Ed Hanway](@ehanway@ilm.com), 2016-07-25) 

*  [Merge pull request #195 from openexr/master](https://github.com/openexr/openexr/commit/bc234de193bd9cd32d94648e2936270aa4406e91) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2016-07-25) Catch develop branch up with commits in master. 

*  [fix potential memory leak](https://github.com/openexr/openexr/commit/d2f10c784d52f841b85e382620100cdbf0d3b1e5) ([CAHEK7](@ghosts.in.a.box@gmail.com), 2016-02-05) 

*  [Fix warnings when compiled with MSVC 2013.](https://github.com/openexr/openexr/commit/3aabef263083024db9e563007d0d76609ac8d585) ([Xo Wang](@xow@google.com), 2016-01-06) Similar fix to that from a27e048451ba3084559634e5e045a92a613b1455. 

*  [Fix typo in C bindings (Close #140)](https://github.com/openexr/openexr/commit/c229dfe63380f41dfae1e977b10dfc7c49c7efc7) ([Edward Kmett](@ekmett@gmail.com), 2015-12-09) IMF_RAMDOM_Y should be IMF_RANDOM_Y 

*  [Fix copy and paste bug](https://github.com/openexr/openexr/commit/501b654d851e2da1d9e5ca010a1e13fe34ae24ab) ([Christopher Kulla](@fpsunflower@users.noreply.github.com), 2015-11-19) The implementation of DeepTiledInputPart::tileXSize was copy and pasted from the function above but not changed. This causes it tor return incorrect values. 

*  [Switch AVX detection asm to not use an empty clobber list for use with older gcc versions](https://github.com/openexr/openexr/commit/51073d1aa8f96963fc6a3ecad8f844ce70c90991) ([Kevin Wheatley](@kevin.wheatley@framestore.com), 2015-10-14) 

*  [Merge pull request #145 from karlrasche/DWAx_clamp_float32](https://github.com/openexr/openexr/commit/521b25df787b460e57d5c1e831b232152b93a6ee) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2015-10-23) Clamp, don't cast, float inputs with DWAx compression 

*  [Merge pull request #143 from karlrasche/DWAx_bad_zigzag_order](https://github.com/openexr/openexr/commit/9547d38199f5db2712c06ccdda9195badbecccaa) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2015-10-23) Wrong zig-zag ordering used for DWAx decode optimization 

*  [Merge pull request #157 from karlrasche/DWAx_compress_bound](https://github.com/openexr/openexr/commit/de27156b77896aeef5b1c99edbca2bc4fa784b51) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2015-10-23) Switch over to use compressBound() instead of manually computing headroom for compress() 

*  [Switch over to use compressBound() instead of manually computing headroom for compress()](https://github.com/openexr/openexr/commit/c9a2e193ce243c66177ddec6be43bc6f655ff78a) ([Karl Rasche](@karl.rasche@dreamworks.com), 2015-02-18) 

*  [Fix a linker error when compiling OpenEXR statically on Linux](https://github.com/openexr/openexr/commit/caa09c1b361e2b152786d9e8b2b90261c9d9a3aa) ([Wenzel Jakob](@wenzel@inf.ethz.ch), 2015-02-02) Linking OpenEXR and IlmBase statically on Linux failed due to interdependencies between Iex and IlmThread. Simply reversing their order in CMakeLists.txt fixes the issue (which only arises on Linux since the GNU linker is particularly sensitive to the order of static libraries) 

*  [Clamp incoming float values to half, instead of simply casting, on encode.](https://github.com/openexr/openexr/commit/cb172eea58b8be078b88eca35f246e12df2de620) ([Karl Rasche](@karl.rasche@dreamworks.com), 2014-11-24) Casting can introduce Infs, which are zero'ed later on, prior to the forward DCT step. This can have the nasty side effect of forcing bright values to zero, instead of clamping them to 65k. 

*  [Remove errant whitespace](https://github.com/openexr/openexr/commit/fc67c8245dbff48e546abae027cc9c80c98b3db1) ([Karl Rasche](@karl.rasche@dreamworks.com), 2014-11-20) 

*  [Use the correct zig-zag ordering when finding choosing between fast-path inverse DCT versions (computing which rows are all zero)](https://github.com/openexr/openexr/commit/b0d0d47b65c5ebcb8c6493aa2238b9f890c4d7fe) ([Karl Rasche](@karl.rasche@dreamworks.com), 2014-11-19) 

*  [Resolve dependency issue building eLut.h/toFloat.h with CMake/Ninja.](https://github.com/openexr/openexr/commit/8eed7012c10f1a835385d750fd55f228d1d35df9) ([Ralph Potter](@r.potter@bath.ac.uk), 2014-11-05) 

*  [Adding rawPixelDataToBuffer() function for access to compressed data read from scanline input files.](https://github.com/openexr/openexr/commit/1f6eddeea176ce773dacd5cdee0cbad0ab549bae) ([Lucy Wilkes](@lucywilkes@users.noreply.github.com), 2014-10-22) Changes from The Foundry to add rawPixelDataToBuffer(...) function to the OpenEXR library. This allows you to read raw scan lines into an external buffer. It's similar to the existing function rawPixelData, but unlike this existing function it allows the user to control where the data will be stored instead of reading it into a local buffer. This means you can store multiple raw scan lines at once and enables the decompression of these scan lines to be done in parallel using an application's own threads. (cherry picked from commit ca76ebb40a3c5a5c8e055f0c8d8be03ca52e91c8) 

*  [Merge pull request #137 from karlrasche/interleaveByte2_sse_bug](https://github.com/openexr/openexr/commit/f4a6d3b9fabd82a11b63abf938e9e32f42d2d6d7) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2014-10-15) Fixing SSE2 byte interleaving path to work with short runs 

*  [Fixing SSE2 byte interleaving path to work with short runs](https://github.com/openexr/openexr/commit/da28ad8cd54dfa3becfdac33872c5b1401a9cc3c) ([Karl Rasche](@karl.rasche@dreamworks.com), 2014-09-08) 

*  [Merge pull request #126 from fnordware/LL_literal](https://github.com/openexr/openexr/commit/91015147e5a6a1914bcb16b12886aede9e1ed065) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2014-08-14) Use LL for 64-bit literals 

*  [Change suffixes to ULL because Int64 is unsigned](https://github.com/openexr/openexr/commit/353cbc2e89c582e07796f01bce8f203e84c8ae46) ([Brendan Bolles](@brendan@fnordware.com), 2014-08-14) As discusses in pull request #126 

*  [Merge pull request #127 from openexr/tarball_contents_fix](https://github.com/openexr/openexr/commit/699b4a62d5de9592d26f581a9cade89fdada7e6a) ([Ed Hanway](@ehanway-ilm@users.noreply.github.com), 2014-08-14) Tarball contents fix 

*  [Add dwa test images to dist (tarball) manifest. Also drop README.win32 from tarball. (Already removed from repo.)](https://github.com/openexr/openexr/commit/cbac202a84b0b0bac0fcd92e5b5c8d634085329e) ([Ed Hanway](@ehanway@ilm.com), 2014-08-14) [New Cmake-centric instructions covering builds for Windows and other platforms to follow.] 

*  [Use LL for 64-bit literals](https://github.com/openexr/openexr/commit/57ecf581d053f5cacf2e8fc3c024490e0bbe536f) ([Brendan Bolles](@brendan@fnordware.com), 2014-08-13) On a 32-bit architecture, these literals are too big for just a long, they need to be long long, otherwise I get an error in GCC.


## Version 2.0.1


* Temporarily turning off optimisation code path
	  (Piotr Stanczyk)
* Added additional tests for future optimisation refactoring
	  (Piotr Stanczyk / Peter Hillman)
* Fixes for StringVectors
	  (Peter Hillman)
* Additional checks for type mismatches
	  (Peter Hillman)
* Fix for Composite Deep Scanline
	  (Brendan Bolles)

## Version 2.0.0

* Updated Documentation
	   (Peter Hillman)
* Updated Namespacing mechanism
	   (Piotr Stanczyk)
* Fixes for succd & predd
	   (Peter Hillman)
* Fixes for FPE control registers
	   (Piotr Stanczyk)
* Additional checks and tests on DeepImages, scanlines and tiles
	   (Peter Hillman)
* Folded in Autodesk read optimisations for RGB(A) files
	  (Pascal Jette, Peter Hillman)
* Updated the bootstrap scripts to use libtoolize if glibtoolize isn't available on darwin. 
	  (Nick Rasmussen)
* Numerous minor fixes, missing includes etc

## Version 2.0.0.beta.1:

* Please read the separate file for v2 additions and changes.
* Added git specific files 
	  (Piotr Stanczyk)
* Updated the so verison to 20
	  (Piotr Stanczyk)
* Initial use of the CMake build system 
	  (Nicholas Yue)

## Version 1.7.1:

* Updated the .so verison to 7.
	  (Piotr Stanczyk)     

## Version 1.7.0:

* Added support for targetting builds on 64bit Windows and minimising 
	  number of compiler warnings on Windows. Thanks to Ger Hobbelt for his
	  contributions to CreateDLL.
	  (Ji Hun Yu)
* Added new atttribute types:
	  M33dAttribute   3x3 double-precision matrix
	  M44dAttribute   4x4 double-precision matrix
	  V2d             2D double-precision vector
	  V3d             3D double-precision vector
	  (Florian Kainz)
* Bug fix: crash when reading a damaged image file (found
	  by Apple).  An exception thrown inside the PIZ Huffman
	  decoder bypasses initialization of an array of pointers.
	  The uninitialized pointers are later passed to operator
	  delete.
	  (Florian Kainz)
* Bug fix: crash when reading a damaged image file (found by
	  Apple).  Computing the size of input certain buffers may
	  overflow and wrap around to a small number, later causing
	  writes beyond the end of the buffer.
	  (Florian Kainz)
* In the "Technical Introduction" document, added
	  Premultiplied vs. Un-Premulitiplied Color section:
	  states explicitly that pixels with zero alpha and non-zero
	  RGB are allowed, points out that preserving such a pixel can
	  be a problem in application programs with un-premultiplied
	  internal image representations.
	  (Florian Kainz)
* exrenvmap improvements:
  - New command line flags set the type of the input image to
	    latitude-longitude map or cube-face map, overriding the
	    envmap attribute in the input file header.
  - Cube-face maps can now be assembled from or split into six
	    square sub-images.
  - Converting a cube-face map into a new cube-face map with
	    the same face size copies the image instead of resampling
	    it.  This avoids blurring when a cube-face map is assembled
	    from or split into sub-images.
	  (Florian Kainz)
* Updated standard chromaticities in ImfAcesFile.cpp to match
	  final ACES (Academy Color Encoding Specification) document.
	  (Florian Kainz)
* Added worldToCamera and worldToNDC matrices to
	  ImfStandardAttributes.h (Florian Kainz)
* Increased the maximum length of attribute and channel names
	  from 31 to 255 characters.  For files that do contain names
	  longer than 31 characters, a new LONG_NAMES_FLAG in the fil
	  version number is set.  This flag causes older versions of
	  the IlmImf library (1.6.1 and earlier) to reject files with
	  long names.  Without the flag, older library versions would
	  mis-interpret files with long names as broken.
	  (Florian Kainz)
* Reading luminance/chroma-encoded files via the RGBA
	  interface is faster: buffer padding avoids cache thrashing
	  for certain image sizes, redundant calls to saturation()
	  have been eliminated.
	  (Mike Wall)
* Added "hemispherical blur" option to exrenvmap.
	  (Florian Kainz)
* Added experimental version of I/O classes for ACES file
	  format (restricted OpenEXR format with special primaries
	  and white point); added exr2aces file converter.
	  (Florian Kainz)
* Added new constructors to classes Imf::RgbaInputFile and
	  Imf::TiledRgbaInputFile.  The new constructors have a
	  layerName parameter, which allows the caller to specify
	  which layer of a multi-layer or multi-view image will
	  be read.
	  (Florian Kainz)
* A number of member functions in classes Imf::Header,
	  Imf::ChannelList and Imf::FrameBuffer have parameters
	  of type "const char *".  Added equivalent functions that
	  take "const std::string &" parameters.
	  (Florian Kainz)
* Added library support for Weta Digital multi-view images:
	  StringVector attribute type, multiView standard attribute
	  of type StringVector, utility functions related to grouping
	  channels into separate views.
	  (Peter Hillman, Florian Kainz)

## Version 1.6.1:

* Removed Windows .suo files from distribution.
	  (Eric Wimmer)
* Bug fix: crashes, memory leaks and file descriptor leaks
	  when reading damaged image files (some reported by Apple,
	  others found by running IlmImfFuzzTest).
	  (Florian Kainz)
* Added new IlmImfFuzzTest program to test how resilient the
	  IlmImf library is with respect broken input files: the program
	  first damages OpenEXR files by partially overwriting them with
	  random data; then it tries to read the damaged files.  If all
	  goes well, the program doesn't crash.
	  (Florian Kainz)

## Version 1.6.0:

* Bumped DSO version number to 6.0
	  (Florian Kainz)
* Added new standard attributes related to color rendering with
	  CTL (Color Transformation Language): renderingTransform,
	  lookModTransform and adoptedNeutral.
	  (Florian Kainz)
* Bug fix: for pixels with luminance near HALF_MIN, conversion
	  from RGB to luminance/chroma produces NaNs and infinities
	  (Florian Kainz)
* Bug fix: excessive desaturation of small details with certain
	  colors after repeatedly loading and saving luminance/chroma
	  encoded images with B44 compression.
	  (Florian Kainz)
* Added B44A compression, a minor variation of B44: in most cases,
	  the compression ratio is 2.28:1, the same as with B44, but in
	  uniform image areas where all pixels have the same value, the
	  compression ratio increases to 10.66:1.  Uniform areas occur, for
	  example, in an image's alpha channel, which typically contains
	  large patches that are solid black or white, or in computer-
	  generated images with a black background.
	  (Florian Kainz)
* Added flag to configure.ac to enable or disable use of large
	  auto arrays in the IlmImf library.  Default is "enable" for
	  Linux, "disable" for everything else.
	  (Darby Johnston, Florian Kainz)
* corrected version number on dso's (libtool) - now 5.0
* Separated ILMBASE_LDFLAGS and ILMBASE_LIBS so that test programs
	  can link with static libraries properly
* eliminated some warning messages during install
	  (Andrew Kunz)
	
## Version 1.5.0:

* reorganized packaging of OpenEXR libraries to facilitate
	  integration with CTL.  Now this library depends on the library
	  IlmBase.  Some functionality has been moved into OpenEXR_Viewers,
	  which depends on two other libraries, CTL and OpenEXR_CTL.
	  Note: previously there were separate releases of
	  OpenEXR-related plugins for Renderman, Shake and Photoshop.
	  OpenEXR is supported natively by Rendermand and Photoshop, so
	  these plugins will not be supported for this or future
	  versions of OpenEXR.
	  (Andrew Kunz)
* New build scripts for Linux/Unix
	  (Andrew Kunz)
* New Windows project files and build scripts
	  (Kimball Thurston)
* float-to-half conversion now preserves the sign of float zeroes
	  and of floats that are so small that they become half zeroes.
	  (Florian Kainz)
* Bug fix: Imath::Frustum<T>::planes() returns incorrect planes
	  if the frustum is orthogonal.
	  (Philip Hubbard)
* added new framesPerSecond optional standard attribute
	  (Florian Kainz)
* Imath cleanup:
  - Rewrote function Imath::Quat<T>::setRotation() to make it
	    numerically more accurate, added confidence tests
  - Rewrote function Imath::Quat<T>::slerp() using Don Hatch's
	    method, which is numerically more accurate, added confidence
	    tests.
  - Rewrote functions Imath::closestPoints(), Imath::intersect(),
	    added confidence tests.
  - Removed broken function Imath::nearestPointOnTriangle().
  - Rewrote Imath::drand48(), Imath::lrand48(), etc. to make
	    them functionally identical with the Unix/Linux versions
	    of drand48(), lrand48() and friends.
  - Replaced redundant definitions of Int64 in Imath and IlmImf
	    with a single definition in ImathInt64.h.
	  (Florian Kainz)
* exrdisplay: if the file's and the display's RGB chromaticities
	  differ, the pixels RGB values are transformed from the file's
	  to the display's RGB space.
	  (Florian Kainz)
* Added new lossy B44 compression method.  HALF channels are
	  compressed with a fixed ratio of 2.28:1.  UINT and FLOAT
	  channels are stored verbatim, without compression.
	  (Florian Kainz)

## Version 1.4.0a:

* Fixed the ReleaseDLL targets for Visual Studio 2003.
	  (Barnaby Robson)
	
## Version 1.4.0:	

* Production release.
* Bug Fix: calling setFrameBuffer() for every scan line
	  while reading a tiled file through the scan line API
	  returns bad pixel data. (Paul Schneider, Florian Kainz)

## Version 1.3.1:

* Fixed the ReleaseDLL targets for Visual Studio 2005.
	  (Nick Porcino, Drew Hess)
* Fixes/enhancements for createDLL.
	  (Nick Porcino)
	
## Version 1.3.0:

* Removed openexr.spec file, it's out of date and broken to
	  boot.
	  (Drew Hess)
* Support for Visual Studio 2005.
	  (Drew Hess, Nick Porcino)
* When compiling against OpenEXR headers on Windows, you
	  no longer need to define any HAVE_* or PLATFORM_* 
	  macros in your projects.  If you are using any OpenEXR
	  DLLs, however, you must define OPENEXR_DLL in your
	  project's preprocessor directives.
	  (Drew Hess)
* Many fixes to the Windows VC7 build system.
	  (Drew Hess, Nick Porcino)
* Support for building universal binaries on OS X 10.4.
	  (Drew Hess, Paul Schneider)
* Minor configure.ac fix to accomodate OS X's automake.
	  (Drew Hess)
* Removed CPU-specific optimizations from configure.ac,
	  autoconf's guess at the CPU type isn't very useful,
	  anyway.  Closes #13429.
	  (Drew Hess)
* Fixed quoting for tests in configure.ac.  Closes #13428.
	  (Drew Hess)
* Use host specification instead of target in configure.ac.
	  Closes #13427.
	  (Drew Hess)
* Fix use of AC_ARG_ENABLE in configure.ac.  Closes
	  #13426.
	  (Drew Hess)
* Removed workaround for OS X istream::read bug.
	  (Drew Hess)
* Added pthread support to OpenEXR pkg-config file.
	  (Drew Hess)
* Added -no-undefined to LDFLAGS and required libs to LIBADD
	  for library projects with other library dependencies, per
	  Rex Dieter's patch.
	  (Drew Hess)
* HAVE_* macros are now defined in the OpenEXRConfig.h header
	  file instead of via compiler flags.  There are a handful of
	  public headers which rely on the value of these macros,
	  and projects including these headers have previously needed
	  to define the same macros and values as used by OpenEXR's
	  'configure', which is bad form.  Now 'configure' writes these
	  values to the OpenEXRConfig.h header file, which is included
	  by any OpenEXR source files that need these macros.  This
	  method of specifying HAVE_* macros guarantees that projects
	  will get the proper settings without needing to add compile-
	  time flags to accomodate OpenEXR.  Note that this isn't
	  implemented properly for Windows yet.
	  (Drew Hess)
* Platform cleanups:
	  - No more support for IRIX or OSF1.
	  - No more explicit support for SunOS, because we have no way to
	    verify that it's working.  I suspect that newish versions of 
	    SunOS will just work out of the box, but let me know if not.
	  - No more PLATFORM_* macros (vestiges of the ILM internal build 
	    system).  PLATFORM_DARWIN_PPC is replaced by HAVE_DARWIN.
	    PLATFORM_REDHAT_IA32 (which was only used in IlmImfTest) is
	    replaced by HAVE_LINUX_PROCFS.
	  - OS X 10.4, which is the minimum version we're going to support
	    with this version, appears to have support for nrand48 and friends,
	    so no need to use the Imath-supplied version of them anymore.
	  (Drew Hess)
* No more PLATFORM_WINDOWS or PLATFORM_WIN32, replace with 
	  proper standard Windows macros.  (Drew Hess)
* Remove support for gcc 2.95, no longer supported.  (Drew Hess)
* Eliminate HAVE_IOS_BASE macro, OpenEXR now requires support for
	  ios_base.  (Drew Hess)
* Eliminate HAVE_STL_LIMITS macro, OpenEXR now requires the ISO C++
	  <limits> header.  (Drew Hess)
* Use double quote-style include dirctives for OpenEXR
	  includes.  (Drew Hess)
* Added a document that gives an overview of the on-disk
	  layout of OpenEXR files (Florian Kainz)
* Added sections on layers and on memory-mapped file input
	  to the documentation.  (Florian Kainz)
* Bug fix: reading an incomplete file causes a deadlock while
	  waiting on a semaphore.  (Florian Kainz)
* Updated documentation (ReadingAndWritingImageFiles.sxw) and
	  sample code (IlmImfExamples):
	  Added a section about multi-threading, updated section on
	  thread-safety, changed documentation and sample code to use
	  readTiles()/writeTiles() instead of readTile()/writeTile()
	  where possible, mentioned that environment maps contain
	  redundant pixels, updated section on testing if a file is
	  an OpenEXR file.
	  (Florian Kainz)
* Multi-threading bug fixes (exceptions could be thrown
	  multiple times, some operations were not thread safe),
	  updated some comments, added comments, more multithreaded
	  testing.
	  (Florian Kainz)
* Added multi-threading support: multiple threads
	  cooperate to read or write a single OpenEXR file.
	  (Wojciech Jarosz)
* Added operator== and operator!= to Imath::Frustum.
	  (Andre Mazzone)
* Bug fix: Reading a PIZ-compressed file with an invalid
	  Huffman code table caused crashes by indexing off the
	  end of an array.
	  (Florian Kainz)

## Version 1.2.2:

* Updated README to remove option for building with Visual C++ 6.0.
	  (Drew Hess)
* Some older versions of gcc don't support a full iomanip
	  implemenation; check for this during configuration. 
	  (Drew Hess)
* Install PDF versions of documentation, remove old/out-of-date
	  HTML documentation.  (Florian Kainz)
* Removed vc/vc6 directory; Visual C++ 6.0 is no longer
	  supported.  (Drew Hess)
* Updated README.win32 with details of new build system.
	  (Florian Kainz, Drew Hess)
* New build system for Windows / Visual C++ 7 builds both
	  static libraries and DLLs.
	  (Nick Porcino)
* Removed Imath::TMatrix<T> and related classes, which are not
	  used anywhere in OpenEXR.
	  (Florian Kainz)
* Added minimal support for "image layers" to class Imf::ChannelList
	  (Florian Kainz)
* Added new isComplete() method to InputFile, TiledInputFile
	  etc., that checks if a file is complete or if any pixels
	  are missing (for example, because writing the file was
	  aborted prematurely).
	  (Florian Kainz)
* Exposed staticInitialize() function in ImfHeader.h in order
	  to allow thread-safe library initialization in multithreaded
	  programs.
	  (Florian Kainz)
* Added a new "time code" attribute
	  (Florian Kainz)
* exrmaketiled: when a MIPMAP_LEVELS or RIPMAP_LEVELS image
	  is produced, low-pass filtering takes samples outside the
	  image's data window.  This requires extrapolating the image.
	  The user can now specify how the image is extrapolated
	  horizontally and vertically (image is surrounded by black /
	  outermost row of pixels repeats / entire image repeats /
	  entire image repeats, every other copy is a mirror image).
	  exrdisplay: added option to swap the top and botton half,
	  and the left and right half of an image, so that the image's
	  four corners end up in the center.  This is useful for checking
	  the seams of wrap-around texture map images.
	  IlmImf library: Added new "wrapmodes" standard attribute
	  to indicate the extrapolation mode for MIPMAP_LEVELS and
	  RIPMAP_LEVELS images.
	  (Florian Kainz)
* Added a new "key code" attribute to identify motion picture
	  film frames.
	  (Florian Kainz)
* Removed #include <Iex.h> from ImfAttribute.h, ImfHeader.h
	  and ImfXdr.h so that including header files such as
	  ImfInputFile.h no longer defines ASSERT and THROW macros,
	  which may conflict with similar macros defined by
	  application programs.
	  (Florian Kainz)
* Converted HTML documentation to OpenOffice format to
	  make maintaining the documents easier:
	      api.html -> ReadingAndWritingImageFiles.sxw
	      details.html -> TechnicalIntroduction.sxw
	  (Florian Kainz)

## Version 1.2.1:

* exrenvmap and exrmaketiled use slightly less memory
	  (Florian Kainz)
* Added functions to IlmImf for quickly testing if a file
	  is an OpenEXR file, and whether the file is scan-line
	  based or tiled. (Florian Kainz)
* Added preview image examples to IlmImfExamples.  Added
	  description of preview images and environment maps to
	  docs/api.html (Florian Kainz)
* Bug fix: PXR24 compression did not work properly for channels
	  with ySampling != 1.
	  (Florian Kainz)
* Made template <class T> become  template <class S, class T> for 
          the transform(ObjectS, ObjectT) methods. This was done to allow
          for differing templated objects to be passed in e.g.  say a 
          Box<Vec3<S> and a Matrix44<T>, where S=float and T=double.
          (Jeff Yost, Arkell Rasiah)
* New method Matrix44::setTheMatrix(). Used for assigning a 
          M44f to a M44d. (Jeff Yost, Arkell Rasiah)
* Added convenience Color typedefs for half versions of Color3
          and Color4. Note the Makefile.am for both Imath and ImathTest
          have been updated with -I and/or -L pathing to Half.
          (Max Chen, Arkell Rasiah)
* Methods equalWithAbsError() and equalWithRelError() are now
          declared as const. (Colette Mullenhoff, Arkell Rasiah)
* Fixes for gcc34. Mainly typename/template/using/this syntax
          correctness changes. (Nick Ramussen, Arkell Rasiah)
* Added Custom low-level file I/O examples to IlmImfExamples
	  and to the docs/api.html document.  (Florian Kainz)
* Eliminated most warnings messages when OpenEXR is compiled
	  with Visual C++.  The OpenEXR code uses lots of (intentional
	  and unintended) implicit type conversions.  By default, Visual
	  C++ warns about almost all of them.  Most implicit conversions
	  have been removed from the .h files, so that including them
	  should not generate warnings even at warning level 3.  Most
	  .cpp files are now compiled with warning level 1.
	  (Florian Kainz)

## Version 1.2.0:

* Production-ready release.
* Disable long double warnings on OS X.  (Drew Hess)
* Add new source files to VC7 IlmImfDll target.  (Drew Hess)
* Iex: change the way that APPEND_EXC and REPLACE_EXC modify
	  their what() string to work around an issue with Visual C++
	  7.1.  (Florian Kainz, Nick Porcino)
* Bumped OpenEXR version to 1.2 and .so versions to 2.0.0 in
	  preparation for the release.  (Drew Hess)
* Imath: fixed ImathTMatrix.h to work with gcc 3.4.  (Drew Hess)
* Another quoting fix in openexr.m4.  (Drew Hess)
* Quoting fix in acinclude.m4 for automake 1.8.  (Brad Hards)
* Imath: put inline at beginning of declaration in ImathMatrix.h
	  to fix a warning.  (Ken McGaugh)
* Imath: made Vec equalWith*Error () methods const.
* Cleaned up compile-time Win32 support.  (Florian Kainz)
* Bug fix: Reading a particular broken PIZ-compressed file
	  caused crashes by indexing off the end of an array.
	  (Florian Kainz)

## Version 1.1.1:

* Half: operator= and variants now return by reference rather
	  than by value.  This brings half into conformance with
	  built-in types.  (Drew Hess)
* Half: remove copy constructor, let compiler supply its
	  own.  This improves performance up to 25% on some
	  expressions using half.  (Drew Hess)
* configure: don't try to be fancy with CXXFLAGS, just use
	  what the user supplies or let configure choose a sensible
	  default if CXXFLAGS is not defined.
* IlmImf: fixed a bug in reading scanline files on big-endian
          architectures.  (Drew Hess)
* exrmaketiled: Added an option to select compression type.
	  (Florian Kainz)
* exrenvmap: Added an option to select compression type.
	  (Florian Kainz)
* exrdisplay: Added some new command-line options.  (Florian Kainz)
* IlmImf: Added Pixar's new "slightly lossy" image compression
	  method.  The new method, named PXR24, preserves HALF and
	  UINT data without loss, but FLOAT pixels are converted to
	  a 24-bit representation.  PXR24 appears to compress
	  FLOAT depth buffers very well without losing much accuracy.
	  (Loren Carpenter, Florian Kainz)
* Changed top-level LICENSE file to allow for other copyright
	  holders for individual files.
* IlmImf: TILED FILE FORMAT CHANGE.  TiledOutputFile was
	  incorrectly interleaving channels and scanlines before
	  passing pixel data to a compressor.  The lossless compressors
	  still work, but lossy compressors do not.  Fix the bug by
	  interleaving channels and scanlines in tiled files in the
	  same way as ScanLineOutputFile does.  Programs compiled with
	  the new version of IlmImf cannot read tiled images produced
	  with version 1.1.0.  (Florian Kainz)
* IlmImf: ImfXdr.h fix for 64-bit architectures.  (Florian Kainz)
* IlmImf: OpenEXR now supports YCA (luminance/chroma/alpha)
	  images with subsampled chroma channels.  When an image
	  is written with the RGBA convenience interface, selecting
	  WRITE_YCA instead of WRITE_RGBA causes the library to
	  convert the pixels to YCA format.  If WRITE_Y is selected,
	  only luminance is stored in the file (for black and white
	  images).  When an image file is read with the RGBA convenience
	  interface, YCA data are automatically converted back to RGBA.
	  (Florian Kainz)
* IlmImf: speed up reading tiled files as scan lines.
	  (Florian Kainz)
* Half:  Fixed subtle bug in Half where signaling float NaNs
	  were being converted to inf in half.  (Florian Kainz)
* gcc 3.3 compiler warning cleanups.  (various)
* Imath: ImathEuler.h fixes for gcc 3.4.  (Garrick Meeker)
	
## Version 1.1.0:

* Added new targets to Visual C++ .NET 2003 project
	  for exrmaketiled, exrenvmap, exrmakepreview, and exrstdattr.
	  (Drew Hess)
* A few assorted Win32 fixes for Imath.  (Drew Hess)
* GNU autoconf builds now produce versioned libraries.
	  This release is 1:0:0.  (Drew Hess)
* Fixes for Visual C++ .NET 2003.  (Paul Schneider)
* Updated Visual C++ zlib project file to zlib 1.2.1.
	  (Drew Hess)
* exrdisplay: Fixed fragment shader version.  (Drew Hess)
* *Test: Fixed some compiler issues.  (Drew Hess)
* Imath: Handle "restrict" keyword properly.  (Drew Hess)
* IlmImfExamples: Updated to latest versions of example
	  source code, includes tiling and multi-res images.
	  (Florian Kainz)
* exrmakepreview: A new utility to create preview images.
	  (Florian Kainz)
* exrenvmap: A new utility to create OpenEXR environment
	  maps.  (Florian Kainz)
* exrstdattr: A new utility to modify standard 
	  attributes.  (Florian Kainz)
* Updated exrheader to print level rounding mode and
	  preview image size.  (Florian Kainz)
* Updated exrmaketiled to use level rounding mode.
	  (Florian Kainz)
* IlmImf: Changed the orientation of lat-long envmaps to
	  match typical panoramic camera setups.  (Florian Kainz)
* IlmImf: Fixed a bug where partially-completed files with
	  DECREASING_Y could not be read.  (Florian Kainz)
* IlmImf: Added support for selectable rounding mode (up/down)
	  when generating multiresolution files.  (Florian Kainz)
* exrdisplay: Support for tiled images, mip/ripmaps, preview
	  images, and display windows.  (Florian Kainz, Drew Hess)
* exrmaketiled: A new utility which generates tiled
	  versions of OpenEXR images.  (Florian Kainz)
* IlmImf: Changed Imf::VERSION to Imf::EXR_VERSION to
	  work around problems with autoconf VERSION macro
	  conflict.  (Drew Hess)
* exrheader: Support for tiles, mipmaps, environment
	  maps.  (Florian Kainz)
* IlmImf: Environment map support.  (Florian Kainz)
* IlmImf: Abstracted stream I/O support.  (Florian Kainz)
* IlmImf: Support for tiled and mip/ripmapped files;
	  requires new file format.  (Wojciech Jarosz, Florian Kainz)
* Imath: TMatrix*, generic 2D matricies and algorithms.
	  (Francesco Callari)
* Imath: major quaternions cleanup.  (Cary Phillips)
* Imath: added GLBegin, GLPushAttrib, GLPushMatrix objects
	  for automatic cleanup on exceptions.  (Cary Phillips)
* Imath: removed implicit scalar->vector promotions and vector
	  comparisons.  (Nick Rasmussen)
	
## Version 1.0.7:

* Fixed a typo in one of the IlmImfTest tests. (Paul Schneider)
* Fixed a bug in exrdisplay that causes the image to display
	  as all black if there's a NaN or infinity in an OpenEXR
	  image. (Florian Kainz)
* Updated exrheader per recent changes to IlmImf library.
	  (Florian Kainz)
* Changed an errant float to a T in ImathFrame.h nextFrame().
	  (Cary Phillips)
* Support for new "optional standard" attributes
	  (chromaticities, luminance, comments, etc.).
	  (Florian Kainz, Greg Ward, Joseph Goldstone)
* Fixed a buffer overrun in ImfOpaqueAttribute. (Paul Schneider)
* Added new function, isImfMagic (). (Florian Kainz)
	
## Version 1.0.6:

* Added README.win32 to disted files.
* Fixed OpenEXR.pc.in pkg-config file, OpenEXR now works
	  with pkg-config.
* Random fixes to readme files for new release.
* Fixed openexr.m4, now looks in /usr by default.
* Added Visual Studio .NET 2003 "solution."
* Fixes for Visual Studio .NET 2003 w/ Microsoft C++ compiler.
	  (Various)
* Random Imath fixes and enhancements.  Note that 
	  extractSHRT now takes an additional optional
          argument, see ImathMatrixAlgo.h for details.  (Various)
* Added Wojciech Jarosz to AUTHORS file.
* Added test cases for uncompressed case, preview images,
	  frame buffer type conversion.  (Wojciech Jarosz,
	  Florian Kainz)
* Fix a bug in IlmImf where uncompressed data doesn't get
	  read/written correctly.  (Wojciech Jarosz)
* Added support for preview images and preview image
	  attributes (thumbnail images) in IlmImf.  (Florian Kainz)
* Added support for automatic frame buffer type conversion
	  in IlmImf.  (Florian Kainz)
* Cleaned up some compile-time checks.
* Added HalfTest unit tests.
* [exrdisplay] Download half framebuffer to texture memory 
	  instead of converting to float first.  Requires latest
	  Nvidia drivers.

## Version 1.0.5:

* Fixed IlmImf.dll to use static runtime libs (Andreas).
* Added exrheader project to Visual Studio 6.0 workspace.
* Added some example code showing how to use the IlmImf library.
	  (Florian)
* Use DLL runtime libs for Win32 libraries rather than static
	  runtime libs.
* Add an exrdisplay_fragshader project to the Visual Studio 6.0
	  workspace to enable fragment shaders in Win32.
* Add an IlmImfDll project to the Visual Studio 6.0 workspace.
* In Win32, export the ImfCRgbaFile C interface via a DLL so
	  that Visual C++ 6.0 users can link against an Intel-compiled
	  IlmImf.  (Andreas Kahler)
* Use auto_ptr in ImfAutoArray on Win32, it doesn't like large 
	  automatic stacks.
* Performance improvements in PIZ decoding, between
	  20 and 60% speedup on Athlon and Pentium 4 systems.
          (Florian)
* Updated the README with various information, made
	  some cosmetic changes for readability.
* Added fragment shader support to exrdisplay.
* Bumped the version to 1.0.5 in prep for release.
* Updated README and README.OSX to talk about CodeWarrior 
          project files.
* Incorporated Rodrigo Damazio's patch for an openexr.m4
	  macro file and an openexr.spec file for building RPMs.
* Small change in ImfAttribute.h to make IlmImf compile with gcc 2.95.
* Updated ImfDoubleAttribute.h for Codewarrior on MacOS.
* Added exrheader utility.
* Update to AUTHORS file.
* Added a README.win32 file.
* Added project files for Visual Studio 6.0.
* Initial Win32 port.  Requires Visual Studio 6.0 and Intel C++
	  compiler version 7.0.
* Added new intersectT method in ImathSphere.h
* Fixed some bugs in ImathQuat.h
* Proper use of fltk-config to get platform-specific FLTK
	  compile- and link-time flags.
* exrdisplay uses Imath::Math<T>::pow instead of powf now.
	  powf is not availble on all platforms.
* Roll OS X "hack" into the source until Apple fixes their
	  istream implementation.
	
## Version 1.0.4:

* OpenEXR is now covered by a modified BSD license.  See LICENSE
	  for the new terms.

## Version 1.0.3:


* OpenEXR is now in sf.net CVS.
* Imf::Xdr namespace cleanups.
* Some IlmImfTest cleanups for OS X.
* Use .cpp extension in exrdisplay sources.
* Iex cleanups.
* Make IlmImf compile with Metrowerks Codewarrior.
* Change large automatic stacks in ImfHuf.C to auto_ptrs allocated
	  off the heap.  MacOS X default stack size isn't large enough.
* std::ios fix for MacOS X in ImfInputFile.C.
* Added new FP predecessor/successor functions to Imath, added
	  tests to ImathTest
* Fixed a bug in Imath::extractSHRT for 3x3 matricies when
	  exactly one of the original scaling factors is negative, updated
	  ImathTest to check this case.
* Install include files when 'make install' is run.
* exrdisplay requires fltk 1.1+ now in an effort to support
	  a MacOS X display program (fltk 1.1 runs on OS X), though this
	  is untested.
* renamed configure.in to configure.ac
* Removed some tests from IexTest that are no longer used.
* Removed ImfHalfXdr.h, it's not used anymore.
* Revamped the autoconf system, added some compile-time 
          optimizations, a pkgconfig target, and some maintainer-specific
          stuff.

## Version 1.0.2:


* More OS X fixes in Imath, IlmImf and IlmImfTest.
* Imath updates.
* Fixed a rotation bug in Imath

## Version 1.0.1:


* Used autoconf 2.53 and automake 1.6 to generate build environment.
* Makefile.am cleanups.
* OS X fixes.
* removed images directory (now distributed separately).

## Version 1.0:


* first official release.
* added some high-level documentation, removed the old OpenEXR.html
          documentation.
* fixed a few nagging build problems.
* bumped IMV_VERSION_NUMBER to 2

## Version 0.9:


* added exrdisplay viewer application.
* cleanup _data in Imf::InputFile and Imf::OutputFile constructors.
* removed old ILM copyright notices.

## Version 0.8:


* Initial release.
