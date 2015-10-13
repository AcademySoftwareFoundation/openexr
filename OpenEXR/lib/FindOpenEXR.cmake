#
# Find OpenEXR include dirs and libraries
#
# Input variables:
#  OpenEXR_ROOT
#  OpenEXR_USE_STATIC_LIBS
#
# Output variables:
#  OpenEXR_FOUND
#  OpenEXR_INCLUDE_PATHS
#  OpenEXR_LIBRARIES
#

include(IlmMacros)

if(OpenEXR_FIND_QUIETLY)
	set(_FIND_PKG_ARG QUIET)
endif()
find_package(IlmBase ${_FIND_PKG_ARG})
find_package(ZLIB ${_FIND_PKG_ARG})

if(IlmBASE_FOUND AND ZLIB_FOUND)
	_ILM_FIND_INCLUDE_DIR(OpenEXR ImfRgbaFile.h)
	if(OpenEXR_INCLUDE_DIR)
		set(OpenEXR_INCLUDE_DIRS ${OpenEXR_INCLUDE_DIR} ${IlmBase_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
	endif()

	set(_openexr_COMPONENTS IlmImf IlmImfUtil)
	_ILM_FIND_LIBRARIES(OpenEXR _openexr_COMPONENTS)
	set(OpenEXR_LIBRARIES ${OpenEXR_LIBRARIES} ${IlmBase_LIBRARIES} ${ZLIB_LIBRARIES})
endif()

# Output results
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR
	REQUIRED_VARS
	OpenEXR_INCLUDE_DIR
	OpenEXR_INCLUDE_DIRS
	${_openexr_LIB_VARS}
	OpenEXR_LIBRARIES
	)

mark_as_advanced(
	OpenEXR_INCLUDE_DIRS
	OpenEXR_LIBRARIES
	)
