# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmaketiled
  makeTiled.cpp
  main.cpp
  Image.cpp
)
SET_ILMBASE_INCLUDE_DIRS( exrmaketiled )

TARGET_LINK_LIBRARIES ( exrmaketiled
  OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
  IlmBase::IlmThread${OPENEXR_TARGET_SUFFIX}
  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
  IlmBase::Half${OPENEXR_TARGET_SUFFIX}
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)

INSTALL ( TARGETS
  exrmaketiled
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
