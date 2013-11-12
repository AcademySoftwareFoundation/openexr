# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrstdattr
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrstdattr
  IlmImf
  IlmThread-2_1
  Iex-2_1
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrstdattr
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
