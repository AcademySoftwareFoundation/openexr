# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrheader
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrheader
  OpenEXR::IlmImf
  IlmBase::Iex
  IlmBase::IlmThread
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrheader
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
