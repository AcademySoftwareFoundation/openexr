# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIEX_EXPORTS)
ENDIF()

ADD_LIBRARY ( PyIex
  PyIex.cpp
  )

TARGET_LINK_LIBRARIES ( PyIex
  ${PYTHON_LIBRARIES}
  )

INSTALL ( TARGETS
  PyIex
  DESTINATION
  lib
)

INSTALL ( FILES 
  PyIex.h
  PyIexExport.h
  PyIexTypeTranslator.h
  DESTINATION
  include/OpenEXR
)

ADD_LIBRARY ( iexmodule
  iexmodule.cpp
  )

SET_TARGET_PROPERTIES (iexmodule PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES ( iexmodule
  PyIex
  ${ILMBASE_LIBRARIES}
  ${Boost_LIBRARIES}
  )

INSTALL ( TARGETS
  iexmodule
  DESTINATION
  lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages
)
