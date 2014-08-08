
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
    PyImathFixedVArray.cpp
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
	Iex${ILMBASE_LIBSUFFIX}
	IexMath${ILMBASE_LIBSUFFIX}
	Imath${ILMBASE_LIBSUFFIX}
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES}
)

INSTALL ( TARGETS PyImath
	DESTINATION
	lib
)

ADD_LIBRARY ( imathmodule ${LIB_TYPE}
    imathmodule.cpp
    PyImathFun.cpp
    PyImathBasicTypes.cpp 
)

SET_TARGET_PROPERTIES ( imathmodule
	PROPERTIES PREFIX "" SUFFIX ".so" BUILD_WITH_INSTALL_RPATH ON
)

TARGET_LINK_LIBRARIES ( imathmodule
	PyImath
	PyIex
	Imath${ILMBASE_LIBSUFFIX}
	Iex${ILMBASE_LIBSUFFIX}
	${Boost_LIBRARIES}
)

INSTALL ( TARGETS imathmodule
	DESTINATION lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages
)

