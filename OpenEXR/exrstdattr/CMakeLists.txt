# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrstdattr main.cpp )
SET_ILMBASE_INCLUDE_DIRS( exrstdattr )

TARGET_LINK_LIBRARIES ( exrstdattr
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrstdattr
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
