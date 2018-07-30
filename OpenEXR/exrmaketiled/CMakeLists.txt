# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmaketiled
  makeTiled.cpp
  main.cpp
  Image.cpp
)
SET_ILMBASE_INCLUDE_DIRS( exrmaketiled )

TARGET_LINK_LIBRARIES ( exrmaketiled
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmaketiled
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
