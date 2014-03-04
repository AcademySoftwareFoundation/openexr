# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmaketiled
  makeTiled.cpp
  main.cpp
  Image.cpp
)

TARGET_LINK_LIBRARIES ( exrmaketiled
  IlmImf
  IlmThread-2_1
  Iex-2_1
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmaketiled
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
