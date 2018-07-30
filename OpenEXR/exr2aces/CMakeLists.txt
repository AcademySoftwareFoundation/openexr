# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exr2aces
  main.cpp
)

TARGET_LINK_LIBRARIES ( exr2aces
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

SET_ILMBASE_INCLUDE_DIRS( exr2aces )

INSTALL ( TARGETS
  exr2aces
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
