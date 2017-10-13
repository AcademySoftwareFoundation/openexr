# pstanczyk@ilm.com

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DIEXMATH_EXPORTS)
ENDIF()
ADD_LIBRARY ( IexMath ${LIB_TYPE}
  IexMathFloatExc.cpp  
  IexMathFpu.cpp
  )
TARGET_LINK_LIBRARIES(IexMath Iex)

IF (OPENEXR_INSTALL)

  INSTALL ( TARGETS
    IexMath
    DESTINATION
    lib
  )

  INSTALL ( FILES 
    IexMathFloatExc.h  
    IexMathFpu.h  
    IexMathIeeeExc.h
    DESTINATION
    include/OpenEXR
  )

ENDIF (OPENEXR_INSTALL)