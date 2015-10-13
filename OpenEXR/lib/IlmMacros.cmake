# Means we don't need to set the openexr directory everywhere
# Uses ${Package}_ROOT
# Sets ${Package}_INCLUDE_DIR
macro(_ILM_FIND_INCLUDE_DIR package header)
	FIND_PATH(${package}_INCLUDE_DIR
		NAMES ${header}
		PATH_SUFFIXES include/OpenEXR
		PATHS ${${package}_ROOT} $ENV{PROGRAMFILES}/openexr
		)
	mark_as_advanced(${package}_INCLUDE_DIR)
endmacro()

# Finds each specified library
# Uses ${Package}_ROOT, ${Package}_USE_STATIC_LIBS
# Sets ${Package}_LIBRARIES
macro(_ILM_FIND_LIBRARIES package components)
	string(TOLOWER ${package} lowerpackage)

	# Adapted from FindBoost.cmake
	# Support preference of static libs by looking for static libs first
	if(${package}_USE_STATIC_LIBS)
		set(_${lowerpackage}_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
		if(WIN32)
			set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
		else()
			set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
		endif()
	endif()

	set(_${lowerpackage}_LIB_VARS)
	set(${package}_LIBRARIES)
	foreach(COMPONENT ${_${lowerpackage}_COMPONENTS})
		string(TOUPPER ${COMPONENT} UPPERCOMPONENT)

		find_library(${package}_${UPPERCOMPONENT}_LIBRARY
			NAMES ${COMPONENT}
			PATHS ${${package}_ROOT}/lib $ENV{PROGRAMFILES}/openexr/lib
			)
		list(APPEND _${lowerpackage}_LIB_VARS ${package}_${UPPERCOMPONENT}_LIBRARY)
		list(APPEND ${package}_LIBRARIES ${${package}_${UPPERCOMPONENT}_LIBRARY})
		mark_as_advanced(${package}_${UPPERCOMPONENT}_LIBRARY)
	endforeach()

	# Revert CMAKE_FIND_LIBRARY_SUFFIXES if changed
	if(${package}_USE_STATIC_LIBS)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${_${lowerpackage}_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
	endif()

	mark_as_advanced(
		${package}_LIBRARIES
		)
endmacro()
