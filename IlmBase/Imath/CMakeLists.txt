# yue.nicholas@gmail.com

ADD_LIBRARY ( Imath
  ImathBox.cpp
  ImathRandom.cpp
  ImathColorAlgo.cpp
  ImathShear.cpp
  ImathFun.cpp
  ImathVec.cpp
  ImathMatrixAlgo.cpp
)
TARGET_LINK_LIBRARIES(Imath Iex)

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIMATH_EXPORTS)
  SET_TARGET_PROPERTIES(Imath PROPERTIES OUTPUT_NAME "Imath-2_0")
ENDIF()

INSTALL ( TARGETS
  Imath
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
