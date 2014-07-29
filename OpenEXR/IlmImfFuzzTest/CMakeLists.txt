# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IlmImfFuzzTest
  fuzzFile.cpp
  main.cpp
  testFuzzDeepTiles.cpp
  testFuzzDeepScanLines.cpp
  testFuzzScanLines.cpp
  testFuzzTiles.cpp
  )

TARGET_LINK_LIBRARIES ( IlmImfFuzzTest 
        IlmImf
        Half
        Iex${ILMBASE_LIBSUFFIX}
        Imath${ILMBASE_LIBSUFFIX}
        IlmThread${ILMBASE_LIBSUFFIX}
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES})

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
