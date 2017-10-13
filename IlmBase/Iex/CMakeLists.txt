# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DIEX_EXPORTS)
ENDIF()

ADD_LIBRARY ( Iex ${LIB_TYPE}
  IexBaseExc.cpp
  IexThrowErrnoExc.cpp
  )

IF (OPENEXR_INSTALL) #D/Labs 

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

  ENDIF (OPENEXR_INSTALL) #D/Labs 