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
        IlmBase::Half
        IlmBase::Imath
        ${PTHREAD_LIB})

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
