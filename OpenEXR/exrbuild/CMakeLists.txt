# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrbuild exrbuild.cpp )
SET_ILMBASE_INCLUDE_DIRS( exrbuild )

TARGET_LINK_LIBRARIES ( exrbuild
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrbuild
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
