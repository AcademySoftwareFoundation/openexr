# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrstdattr
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrstdattr
  IlmImf
  ${ILMBASE_LIBS}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrstdattr
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
