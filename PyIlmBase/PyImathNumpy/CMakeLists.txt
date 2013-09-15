# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIEX_EXPORTS)
ENDIF()

ADD_LIBRARY ( imathnumpymodule
  imathnumpymodule.cpp
  )

SET_TARGET_PROPERTIES (imathnumpymodule PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES ( imathnumpymodule
  Iex-2_0
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  )

INSTALL ( TARGETS
  imathnumpymodule
  DESTINATION
  lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-package
)
