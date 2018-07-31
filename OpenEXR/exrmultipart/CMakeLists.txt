# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultipart exrmultipart.cpp )
SET_ILMBASE_INCLUDE_DIRS( exrmultipart )

TARGET_LINK_LIBRARIES ( exrmultipart
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmultipart
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
