# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultipart
  exrmultipart.cpp
)

TARGET_LINK_LIBRARIES ( exrmultipart
  IlmImf
  ${IlmBase_LIBRARIES}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmultipart
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
