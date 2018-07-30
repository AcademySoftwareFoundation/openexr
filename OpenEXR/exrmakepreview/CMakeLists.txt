# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmakepreview
  makePreview.cpp
  main.cpp
)
SET_ILMBASE_INCLUDE_DIRS( exrmakepreview )


TARGET_LINK_LIBRARIES ( exrmakepreview
  OpenEXR::IlmImf
  IlmBase::IlmThread
  IlmBase::Iex
  IlmBase::Half
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmakepreview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
