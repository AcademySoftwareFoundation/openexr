# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrstdattr
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrstdattr
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half${ILMBASE_LIBSUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrstdattr
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
