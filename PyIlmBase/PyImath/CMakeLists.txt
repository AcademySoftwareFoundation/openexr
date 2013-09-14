# yue.nicholas@gmail.com

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DIEX_EXPORTS)
ENDIF()

LINK_DIRECTORIES( ${ILMBASE_LIBRARY_DIRS})

ADD_LIBRARY ( PyImath ${LIB_TYPE}
  PyImath.cpp
  PyImathAutovectorize.cpp
  PyImathBox2Array.cpp
  PyImathBox3Array.cpp
  PyImathBox.cpp
  PyImathColor3.cpp
  PyImathColor4.cpp
  PyImathEuler.cpp
  PyImathFixedArray.cpp
  PyImathFrustum.cpp
  PyImathLine.cpp
  PyImathMatrix33.cpp
  PyImathMatrix44.cpp
  PyImathPlane.cpp
  PyImathQuat.cpp
  PyImathRandom.cpp
  PyImathShear.cpp
  PyImathStringArray.cpp
  PyImathStringTable.cpp
  PyImathTask.cpp
  PyImathUtil.cpp
  PyImathVec2fd.cpp
  PyImathVec2si.cpp
  PyImathVec3fd.cpp
  PyImathVec3siArray.cpp
  PyImathVec3si.cpp
  PyImathVec4fd.cpp
  PyImathVec4siArray.cpp
  PyImathVec4si.cpp
  )

TARGET_LINK_LIBRARIES ( PyImath
  Imath-2_0
  IexMath-2_0
  Iex-2_0
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  )

INSTALL ( TARGETS
  PyImath
  DESTINATION
  lib
)

INSTALL ( FILES
  PyImath.h
  PyImathAutovectorize.h
  PyImathBoxArrayImpl.h
  PyImathBox.h
  PyImathColor3ArrayImpl.h
  PyImathColor4Array2DImpl.h
  PyImathColor4ArrayImpl.h
  PyImathColor.h
  PyImathDecorators.h
  PyImathEuler.h
  PyImathExport.h
  PyImathFixedArray2D.h
  PyImathFixedArray.h
  PyImathFixedMatrix.h
  PyImathFrustum.h
  PyImathLine.h
  PyImathMathExc.h
  PyImathMatrix.h
  PyImathOperators.h
  PyImathPlane.h
  PyImathQuat.h
  PyImathRandom.h
  PyImathShear.h
  PyImathStringArray.h
  PyImathStringArrayRegister.h
  PyImathStringTable.h
  PyImathTask.h
  PyImathUtil.h
  PyImathVec2Impl.h
  PyImathVec3ArrayImpl.h
  PyImathVec3Impl.h
  PyImathVec4ArrayImpl.h
  PyImathVec4Impl.h
  PyImathVec.h
  PyImathVecOperators.h
  DESTINATION
  include/OpenEXR
)

ADD_LIBRARY ( imathmodule ${LIB_TYPE}
  imathmodule.cpp
  PyImathFun.cpp
  PyImathBasicTypes.cpp
  )

SET_TARGET_PROPERTIES (imathmodule PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES ( imathmodule
  PyImath
  PyIex
  Imath-2_0
  IexMath-2_0
  Iex-2_0
  ${Boost_LIBRARIES}
  ${PYTHON_LIBRARIES}
  )

INSTALL ( TARGETS
  imathmodule
  DESTINATION
  lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-package
)
