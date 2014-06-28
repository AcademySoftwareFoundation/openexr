# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exr2aces
  main.cpp
)

TARGET_LINK_LIBRARIES ( exr2aces
  IlmImf
  IlmThread-2_2
  Iex-2_2
  Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exr2aces
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
