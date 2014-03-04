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
        Iex-2_1
        Imath-2_1
        IlmThread-2_1
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES})

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
