
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

target_compile_definitions( PyImath PRIVATE -DPYIMATH_BUILD )

IF (WIN32)
  target_compile_definitions( PyImath
    PRIVATE OPENEXR_DLL)
ENDIF()



SET_ILMBASE_INCLUDE_DIRS( PyImath )

#    IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
TARGET_LINK_LIBRARIES ( PyImath
    IlmBase::IexMath${OPENEXR_TARGET_SUFFIX}
    IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
    PyIex
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES}
)

INSTALL ( TARGETS PyImath
	DESTINATION
	lib
)

INSTALL (DIRECTORY ./
    DESTINATION include/OpenEXR
    FILES_MATCHING PATTERN "*.h"
)

# must be shared
ADD_LIBRARY ( imathmodule SHARED
    imathmodule.cpp
    PyImathFun.cpp
    PyImathBasicTypes.cpp
)

IF (WIN32)
    SET_TARGET_PROPERTIES ( imathmodule
        PROPERTIES
            PREFIX ""
            OUTPUT_NAME "imath"
            SUFFIX ".pyd"
    )
ELSE ()
    SET_TARGET_PROPERTIES ( imathmodule
        PROPERTIES PREFIX "" SUFFIX ".so" BUILD_WITH_INSTALL_RPATH ON
    )
ENDIF ()

SET_ILMBASE_INCLUDE_DIRS( imathmodule )

#	IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
#	IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
TARGET_LINK_LIBRARIES ( imathmodule
	PyImath
	PyIex
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES}
)

INSTALL ( TARGETS imathmodule
	DESTINATION lib/python${OPENEXR_PYTHON_MAJOR}.${OPENEXR_PYTHON_MINOR}/site-packages
)

