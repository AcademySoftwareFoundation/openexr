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

IF ( OPENEXR_RUN_FUZZ_TESTS )
  ADD_TEST ( TestIlmImfFuzz IlmImfFuzzTest )
  # This takes a really long time to run... the default timeout is 1500 seconds or
  # but this can take 10 hours to run on an older machine
  SET_TESTS_PROPERTIES ( TestIlmImfFuzz PROPERTIES TIMEOUT 36000 )
ENDIF ()

ADD_CUSTOM_TARGET(fuzz ${CMAKE_CURRENT_BINARY_DIR}/IlmImfFuzzTest)
# the depends you can add in ADD_CUSTOM_TARGET only works for other add_custom_command
ADD_DEPENDENCIES(fuzz IlmImfFuzzTest)
