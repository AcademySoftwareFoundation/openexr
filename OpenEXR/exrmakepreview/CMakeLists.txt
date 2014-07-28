# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmakepreview
  makePreview.cpp
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrmakepreview
  IlmImf
  IlmThread${ILMBASE_LIBSUFFIX}
  Iex${ILMBASE_LIBSUFFIX}
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmakepreview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
