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
        OpenEXR::IlmImf
        IlmBase::Half
        IlmBase::Iex
        IlmBase::Imath
        IlmBase::IlmThread
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES})

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
