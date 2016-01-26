# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DIMATH_EXPORTS)
ENDIF()

ADD_LIBRARY ( Imath ${LIB_TYPE}
  ImathBox.cpp
  ImathRandom.cpp
  ImathColorAlgo.cpp
  ImathShear.cpp
  ImathFun.cpp
  ImathVec.cpp
  ImathMatrixAlgo.cpp
)
TARGET_LINK_LIBRARIES(Imath Iex)
GET_FILENAME_COMPONENT ( ILMBASE_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY )
TARGET_INCLUDE_DIRECTORIES ( Imath
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${ILMBASE_DIR}/Half>
  $<INSTALL_INTERFACE:include/OpenEXR>
  )

INSTALL ( TARGETS
  Imath
  EXPORT
  IlmBaseTargets
  DESTINATION
  lib
)

INSTALL ( FILES 
  ImathBoxAlgo.h
  ImathBox.h
  ImathColorAlgo.h
  ImathColor.h
  ImathEuler.h
  ImathExc.h
  ImathExport.h
  ImathForward.h
  ImathFrame.h
  ImathFrustum.h
  ImathFrustumTest.h
  ImathFun.h
  ImathGL.h
  ImathGLU.h
  ImathHalfLimits.h
  ImathInt64.h
  ImathInterval.h
  ImathLimits.h
  ImathLineAlgo.h
  ImathLine.h
  ImathMath.h
  ImathMatrixAlgo.h
  ImathMatrix.h
  ImathNamespace.h
  ImathPlane.h
  ImathPlatform.h
  ImathQuat.h
  ImathRandom.h
  ImathRoots.h
  ImathShear.h
  ImathSphere.h
  ImathVecAlgo.h
  ImathVec.h
  DESTINATION
  include/OpenEXR
)
