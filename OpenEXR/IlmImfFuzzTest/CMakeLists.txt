# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IlmImfFuzzTest
  fuzzFile.cpp
  main.cpp
  testFuzzDeepTiles.cpp
  testFuzzDeepScanLines.cpp
  testFuzzScanLines.cpp
  testFuzzTiles.cpp
  )

SET_ILMBASE_INCLUDE_DIRS( IlmImfFuzzTest )

TARGET_LINK_LIBRARIES ( IlmImfFuzzTest
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB} ${ZLIB_LIBRARIES})

ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
