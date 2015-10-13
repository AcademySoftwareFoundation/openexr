#
# Find IlmBase include dirs and libraries
#
# Input variables:
#  IlmBase_ROOT
#  IlmBase_USE_STATIC_LIBS
#
# Output variables:
#  IlmBase_FOUND
#  IlmBase_INCLUDE_DIRS
#  IlmBase_LIBRARIES
#

include(IlmMacros)

_ILM_FIND_INCLUDE_DIR(IlmBase IlmBaseConfig.h)
if(IlmBase_INCLUDE_DIR)
	set(IlmBase_INCLUDE_DIRS ${IlmBase_INCLUDE_DIR})
endif()

# These components have inter-dependencies
# so we don't allow the user to select individual components
set(_ilmbase_COMPONENTS Half Iex IexMath IlmThread Imath)
_ILM_FIND_LIBRARIES(IlmBase _ilmbase_COMPONENTS)

# Output results
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IlmBase
	REQUIRED_VARS
	IlmBase_INCLUDE_DIR
	IlmBase_INCLUDE_DIRS
	${_ilmbase_LIB_VARS}
	IlmBase_LIBRARIES
	)

mark_as_advanced(
	IlmBase_INCLUDE_DIRS
	IlmBase_LIBRARIES
	)
