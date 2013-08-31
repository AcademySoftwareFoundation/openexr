# yue.nicholas@gmail.com

ADD_LIBRARY ( Iex ${LIB_TYPE}
  IexBaseExc.cpp
  IexThrowErrnoExc.cpp
  )

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIEX_EXPORTS)
  SET_TARGET_PROPERTIES(Iex PROPERTIES OUTPUT_NAME "Iex-2_0")
ENDIF()

INSTALL ( TARGETS
  Iex
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
