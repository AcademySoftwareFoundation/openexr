
# IlmImfUtilTest

LINK_DIRECTORIES ( ${CMAKE_CURRENT_BINARY_DIR}/../IlmImf )


ADD_EXECUTABLE ( IlmImfUtilTest
  main.cpp
  testFlatImage.cpp
  testDeepImage.cpp
  testIO.cpp
 )

 SET_ILMBASE_INCLUDE_DIRS( IlmImfUtilTest )

ADD_TEST ( TestIlmImfUtil IlmImfUtilTest )
TARGET_LINK_LIBRARIES ( IlmImfUtilTest
  OpenEXR::IlmImfUtil${OPENEXR_TARGET_SUFFIX}
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
  )
