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


INSTALL ( TARGETS
  Imath
  DESTINATION
  lib
)

INSTALL ( FILES 
  ImathColorAlgo.h
  ImathMatrixAlgo.h
  ImathVec.h
  ImathShear.h
  ImathFun.h
  ImathBox.h
  ImathBoxAlgo.h
  ImathEuler.h
  ImathExc.h
  ImathLimits.h
  ImathLine.h
  ImathLineAlgo.h
  ImathMatrix.h
  ImathPlane.h
  ImathSphere.h
  ImathVecAlgo.h
  ImathQuat.h
  ImathFrustum.h
  ImathMath.h
  ImathGL.h
  ImathColor.h
  ImathRandom.h
  ImathRoots.h
  ImathHalfLimits.h
  ImathInterval.h
  ImathGLU.h
  ImathFrame.h
  ImathPlatform.h
  ImathInt64.h
  ImathForward.h
  ImathNamespace.h
  ImathExport.h
  DESTINATION
  include/OpenEXR
)
