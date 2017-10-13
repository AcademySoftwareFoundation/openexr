# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultipart
  exrmultipart.cpp
)

TARGET_LINK_LIBRARIES ( exrmultipart
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

IF (OPENEXR_INSTALL)
  INSTALL ( TARGETS
    exrmultipart
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
  )
ENDIF(OPENEXR_INSTALL)
