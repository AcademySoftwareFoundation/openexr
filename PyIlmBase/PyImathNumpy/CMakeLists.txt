
ADD_LIBRARY ( imathnumpymodule ${LIB_TYPE}
    imathnumpymodule.cpp
)

IF (WIN32)
    SET_TARGET_PROPERTIES (imathnumpymodule
        PROPERTIES
            PREFIX ""
            OUTPUT_NAME "imathnumpy"
            SUFFIX ".pyd"
    )
ELSE ()
    SET_TARGET_PROPERTIES (imathnumpymodule
        PROPERTIES PREFIX "" SUFFIX ".so" BUILD_WITH_INSTALL_RPATH ON
    )
ENDIF ()

INCLUDE_DIRECTORIES (
	${NUMPY_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES ( imathnumpymodule
	PyImath
	PyIex
	Imath${ILMBASE_LIBSUFFIX}
	Iex${ILMBASE_LIBSUFFIX}
	${Boost_LIBRARIES}
)

INSTALL ( TARGETS imathnumpymodule
	DESTINATION lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages
)

