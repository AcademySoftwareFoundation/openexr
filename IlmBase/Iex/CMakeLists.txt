# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DIEX_EXPORTS)
ENDIF()

ADD_LIBRARY ( Iex ${LIB_TYPE}
  IexBaseExc.cpp
  IexThrowErrnoExc.cpp
  )

TARGET_INCLUDE_DIRECTORIES ( Iex
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/OpenEXR>
  )

INSTALL ( TARGETS
  Iex
  EXPORT
  IlmBaseTargets
  DESTINATION
  lib
)

INSTALL ( FILES 
  IexBaseExc.h
  IexMathExc.h
  IexThrowErrnoExc.h
  IexErrnoExc.h
  IexMacros.h
  Iex.h
  IexNamespace.h
  IexExport.h
  IexForward.h
  DESTINATION
  include/OpenEXR
)
