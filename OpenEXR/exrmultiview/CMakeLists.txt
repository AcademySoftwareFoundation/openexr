# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)

SET_ILMBASE_INCLUDE_DIRS( exrmultiview )

TARGET_LINK_LIBRARIES ( exrmultiview
  OpenEXR::IlmImf
  IlmBase::Half
  IlmBase::Imath
  IlmBase::Iex
  IlmBase::IlmThread
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmultiview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
