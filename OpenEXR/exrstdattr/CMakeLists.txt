# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrstdattr main.cpp )
SET_ILMBASE_INCLUDE_DIRS( exrstdattr )

TARGET_LINK_LIBRARIES ( exrstdattr
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrstdattr
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
