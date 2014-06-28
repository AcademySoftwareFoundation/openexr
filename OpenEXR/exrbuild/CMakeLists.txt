# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrbuild
  exrbuild.cpp
)

TARGET_LINK_LIBRARIES ( exrbuild
  IlmImf
  IlmThread-2_2
  Iex-2_2
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrbuild
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
