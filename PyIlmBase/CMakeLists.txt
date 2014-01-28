CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( PyIlmBase )

FIND_PACKAGE ( Boost REQUIRED COMPONENTS python )
FIND_PACKAGE ( PythonLibs )

INCLUDE_DIRECTORIES ( ${Boost_INCLUDE_DIR} )
INCLUDE_DIRECTORIES ( ${PYTHON_INCLUDE_DIR} )
INCLUDE_DIRECTORIES ( PyIex )
INCLUDE_DIRECTORIES ( PyImath )
INCLUDE_DIRECTORIES ( ${ILMBASE_PREFIX}/include/OpenEXR )
INCLUDE_DIRECTORIES ( ${NUMPY_INCLUDE_DIR} )

ADD_LIBRARY ( PyIex SHARED 
  ./PyIex/PyIex.cpp
  )

ADD_LIBRARY ( PyImath SHARED
  ./PyImath/PyImathColor4.cpp
  ./PyImath/PyImathMatrix44.cpp
  ./PyImath/PyImathBasicTypes.cpp
  ./PyImath/PyImathBox3Array.cpp
  ./PyImath/PyImathFun.cpp
  ./PyImath/PyImathUtil.cpp
  ./PyImath/PyImath.cpp
  ./PyImath/PyImathMatrix33.cpp
  ./PyImath/PyImathVec3si.cpp
  ./PyImath/PyImathVec2si.cpp
  ./PyImath/PyImathStringArray.cpp
  ./PyImath/PyImathBox.cpp
  ./PyImath/PyImathAutovectorize.cpp
  ./PyImath/PyImathQuat.cpp
  ./PyImath/PyImathColor3.cpp
  ./PyImath/PyImathVec3fd.cpp
  ./PyImath/PyImathVec3siArray.cpp
  ./PyImath/PyImathVec4si.cpp
  ./PyImath/PyImathFixedVArray.cpp
  ./PyImath/PyImathVec4fd.cpp
  ./PyImath/PyImathBox2Array.cpp
  ./PyImath/PyImathFrustum.cpp
  ./PyImath/PyImathStringTable.cpp
  ./PyImath/PyImathVec4siArray.cpp
  ./PyImath/PyImathPlane.cpp
  ./PyImath/PyImathLine.cpp
  ./PyImath/PyImathTask.cpp
  ./PyImath/PyImathEuler.cpp
  ./PyImath/PyImathShear.cpp
  ./PyImath/PyImathFixedArray.cpp
  ./PyImath/PyImathRandom.cpp
  ./PyImath/PyImathVec2fd.cpp
  )

ADD_LIBRARY ( iexmodule SHARED
  ./PyIex/iexmodule.cpp
  )

ADD_LIBRARY ( imathmodule SHARED
  ./PyImath/imathmodule.cpp
  )

ADD_LIBRARY ( imathnumpymodule SHARED
  ./PyImathNumpy/imathnumpymodule.cpp
  )

SET_TARGET_PROPERTIES (
  iexmodule
  imathmodule
  imathnumpymodule
  PROPERTIES
  PREFIX ""
  )

# Installation
INSTALL ( TARGETS
  PyIex
  PyImath
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/lib
  )

# MESSAGE ( "PYTHONLIBS_VERSION_STRING = ${PYTHONLIBS_VERSION_STRING}" )
STRING(SUBSTRING ${PYTHONLIBS_VERSION_STRING} 0 3 PYTHON_MAJOR_MINOR_VERSION)
# MESSAGE ( "PYTHON_MAJOR_MINOR_VERSION = ${PYTHON_MAJOR_MINOR_VERSION}" )

INSTALL ( TARGETS
  iexmodule
  imathmodule
  imathnumpymodule
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/lib/python$${PYTHON_MAJOR_MINOR_VERSION}/site-packages/
  )

INSTALL ( FILES
  pyilmbase.m4
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/share/aclocal
  )

INSTALL ( FILES
  ./PyIex/PyIexExport.h
  ./PyIex/PyIex.h
  ./PyIex/PyIexTypeTranslator.h
  ./PyImath/PyImathAutovectorize.h
  ./PyImath/PyImathBoxArrayImpl.h
  ./PyImath/PyImathBox.h
  ./PyImath/PyImathColor3ArrayImpl.h
  ./PyImath/PyImathColor4Array2DImpl.h
  ./PyImath/PyImathColor4ArrayImpl.h
  ./PyImath/PyImathColor.h
  ./PyImath/PyImathDecorators.h
  ./PyImath/PyImathEuler.h
  ./PyImath/PyImathExport.h
  ./PyImath/PyImathFixedArray2D.h
  ./PyImath/PyImathFixedArray.h
  ./PyImath/PyImathFixedMatrix.h
  ./PyImath/PyImathFixedVArray.h
  ./PyImath/PyImathFrustum.h
  ./PyImath/PyImath.h
  ./PyImath/PyImathLine.h
  ./PyImath/PyImathMathExc.h
  ./PyImath/PyImathMatrix.h
  ./PyImath/PyImathOperators.h
  ./PyImath/PyImathPlane.h
  ./PyImath/PyImathQuat.h
  ./PyImath/PyImathRandom.h
  ./PyImath/PyImathShear.h
  ./PyImath/PyImathStringArray.h
  ./PyImath/PyImathStringArrayRegister.h
  ./PyImath/PyImathStringTable.h
  ./PyImath/PyImathTask.h
  ./PyImath/PyImathUtil.h
  ./PyImath/PyImathVec2Impl.h
  ./PyImath/PyImathVec3ArrayImpl.h
  ./PyImath/PyImathVec3Impl.h
  ./PyImath/PyImathVec4ArrayImpl.h
  ./PyImath/PyImathVec4Impl.h
  ./PyImath/PyImathVec.h
  ./PyImath/PyImathVecOperators.h
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
  )
