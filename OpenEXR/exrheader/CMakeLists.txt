# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrheader
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrheader
  IlmImf
  Iex${ILMBASE_LIBSUFFIX}
  IlmThread${ILMBASE_LIBSUFFIX}
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrheader
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
