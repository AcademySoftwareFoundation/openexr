# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrbuild
  exrbuild.cpp
)

TARGET_LINK_LIBRARIES ( exrbuild
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half${ILMBASE_LIBSUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrbuild
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
