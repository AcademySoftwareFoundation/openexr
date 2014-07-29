# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmaketiled
  makeTiled.cpp
  main.cpp
  Image.cpp
)

TARGET_LINK_LIBRARIES ( exrmaketiled
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmaketiled
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
