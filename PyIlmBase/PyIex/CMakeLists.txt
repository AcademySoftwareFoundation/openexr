
# can't re-use compile, so no sense for object library here...

if(TARGET Python2::Python AND TARGET Boost::${PYILMBASE_BOOST_PY2_COMPONENT})
  Python2_add_library(iexmodule_p2 MODULE PyIex.cpp iexmodule.cpp)
  target_link_libraries(iexmodule_p2
    PRIVATE IlmBase::Iex IlmBase::IexMath
    PRIVATE Boost::${PYILMBASE_BOOST_PY2_COMPONENT})
endif()

if(TARGET Python3::Python AND TARGET Boost::${PYILMBASE_BOOST_PY3_COMPONENT})
  Python3_add_library(iexmodule_p3 MODULE PyIex.cpp iexmodule.cpp)
  target_link_libraries(iexmodule_p3
    PRIVATE IlmBase::Iex IlmBase::IexMath
    PRIVATE Boost::${PYILMBASE_BOOST_PY3_COMPONENT})
endif()

#[==[
ADD_LIBRARY ( PyIex ${LIB_TYPE} PyIex.cpp)

target_compile_definitions( PyIex PRIVATE PYIEX_BUILD )

IF (WIN32)
  target_compile_definitions( PyIex
    PRIVATE OPENEXR_DLL)
ENDIF()

SET_ILMBASE_INCLUDE_DIRS( PyIex )

TARGET_LINK_LIBRARIES ( PyIex
	PUBLIC  IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
	PRIVATE ${PYTHON_LIBRARIES}
	PRIVATE ${Boost_LIBRARIES}
)

INSTALL ( TARGETS PyIex
	DESTINATION
	lib
)

INSTALL (
  FILES
    PyIex.h
    PyIexExport.h
    PyIexTypeTranslator.h

  DESTINATION
    include/OpenEXR
)

# must be shared
ADD_LIBRARY ( iexmodule SHARED
	iexmodule.cpp
)

if (WIN32)
	SET_TARGET_PROPERTIES ( iexmodule
		PROPERTIES
			PREFIX ""
			OUTPUT_NAME "iex"
			SUFFIX ".pyd"
	)
else()
	SET_TARGET_PROPERTIES ( iexmodule
		PROPERTIES PREFIX "" SUFFIX ".so" BUILD_WITH_INSTALL_RPATH ON
	)
endif()

#    IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
TARGET_LINK_LIBRARIES ( iexmodule
	PyIex
	${PYTHON_LIBRARIES}
	${Boost_LIBRARIES}
)

SET_ILMBASE_INCLUDE_DIRS( iexmodule )

INSTALL ( TARGETS iexmodule
	DESTINATION lib/python${OPENEXR_PYTHON_MAJOR}.${OPENEXR_PYTHON_MINOR}/site-packages
)

#]==]
