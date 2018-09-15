# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmakepreview
  makePreview.cpp
  main.cpp
)
SET_ILMBASE_INCLUDE_DIRS( exrmakepreview )


TARGET_LINK_LIBRARIES ( exrmakepreview
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmakepreview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
