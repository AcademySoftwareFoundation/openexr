# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exr2aces
  main.cpp
)

TARGET_LINK_LIBRARIES ( exr2aces
  IlmImf
  ${ILMBASE_LIBS}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exr2aces
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
