# pstanczyk@ilm.com

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DIEXMATH_EXPORTS)
ENDIF()
ADD_LIBRARY ( IexMath ${LIB_TYPE}
  IexMathFloatExc.cpp  
  IexMathFpu.cpp
  )
TARGET_LINK_LIBRARIES(IexMath Iex)
TARGET_INCLUDE_DIRECTORIES ( IexMath
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/OpenEXR>
  )

INSTALL ( TARGETS
  IexMath
  EXPORT
  IlmBaseTargets
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
