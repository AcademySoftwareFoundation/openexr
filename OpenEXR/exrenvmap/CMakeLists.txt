# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrenvmap
  makeLatLongMap.cpp
  readInputImage.cpp
  resizeImage.cpp
  makeCubeMap.cpp
  main.cpp
  blurImage.cpp
  EnvmapImage.cpp
)

TARGET_LINK_LIBRARIES ( exrenvmap
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrenvmap
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
