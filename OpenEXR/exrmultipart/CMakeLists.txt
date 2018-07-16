# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultipart
  exrmultipart.cpp
)

TARGET_LINK_LIBRARIES ( exrmultipart
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmultipart
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
