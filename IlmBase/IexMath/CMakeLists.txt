# pstanczyk@ilm.com

ADD_LIBRARY ( IexMath
  IexMathFloatExc.cpp  
  IexMathFpu.cpp
  )

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIEXMATH_EXPORTS)
  SET_TARGET_PROPERTIES(IexMath PROPERTIES OUTPUT_NAME "IexMath-2_0")
ENDIF()

TARGET_LINK_LIBRARIES(IexMath Iex)

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
