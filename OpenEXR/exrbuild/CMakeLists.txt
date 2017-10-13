# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrbuild
  exrbuild.cpp
)

TARGET_LINK_LIBRARIES ( exrbuild
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

IF (OPENEXR_INSTALL)
  INSTALL ( TARGETS
    exrbuild
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
  )
ENDIF(OPENEXR_INSTALL)