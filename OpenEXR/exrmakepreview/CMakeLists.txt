# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmakepreview
  makePreview.cpp
  main.cpp
)

TARGET_LINK_LIBRARIES ( exrmakepreview
  IlmImf
  IlmThread-2_1
  Iex-2_1
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmakepreview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
