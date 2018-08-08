
# must be shared
ADD_LIBRARY ( imathnumpymodule SHARED
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
SET_ILMBASE_INCLUDE_DIRS( imathnumpymodule )

#	IlmBase::Imath${OPENEXR_TARGET_SUFFIX}
#	IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
TARGET_LINK_LIBRARIES ( imathnumpymodule
	PyImath
	PyIex
	${Boost_LIBRARIES}
)

INSTALL ( TARGETS imathnumpymodule
	DESTINATION lib/python${OPENEXR_PYTHON_MAJOR}.${OPENEXR_PYTHON_MINOR}/site-packages
)

