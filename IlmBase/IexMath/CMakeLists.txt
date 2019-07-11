ilmbase_define_library(IexMath
  PRIV_EXPORT IEXMATH_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    IexMathFloatExc.cpp
    IexMathFpu.cpp
  HEADERS
    IexMathFloatExc.h
    IexMathFpu.h
    IexMathIeeeExc.h
  DEPENDENCIES
    IlmBase::Iex
  )
