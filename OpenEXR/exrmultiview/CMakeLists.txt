# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)

SET_ILMBASE_INCLUDE_DIRS( exrmultiview )

TARGET_LINK_LIBRARIES ( exrmultiview
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmultiview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
