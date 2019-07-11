# "More Modern" CMake version
cmake_minimum_required(VERSION 3.12)

set(CMAKE_PREFIX_PATH "/prefix")

# An "official" way to make this a super-project
# basically overrides the find_package to not find anything
# for stuff we're including locally
set(as_subproject IlmBase OpenEXR)
macro(find_package)
  if(NOT "${ARGV0}" IN_LIST as_subproject)
    _find_package(${ARGV})
  endif()
endmacro()

enable_testing()

add_subdirectory(IlmBase)
# TODO:
#add_subdirectory(PyIlmBase)
add_subdirectory(OpenEXR)
# TODO:
#add_subdirectory(OpenEXR_Viewers)

#[==[

# Packages

if (OPENEXR_BUILD_PYTHON_LIBS)
  find_package(PythonLibs ${OPENEXR_PYTHON_MAJOR}.${OPENEXR_PYTHON_MINOR})
  if(NOT PythonLibs_FOUND)
    message(WARNING "Make Python ${OPENEXR_PYTHON_MAJOR}.${OPENEXR_PYTHON_MINOR} available to CMake's search path, and re-run configuration")
    message(FATAL_ERROR "Python is a required dependency when OPENEXR_BUILD_PYTHON_LIBS is set")
  endif()

  find_package(Boost)
  # Boost.Python use a new versioning scheme starting with Boost 1.67
  if(Boost_VERSION VERSION_GREATER_EQUAL 106700)
    find_package(Boost COMPONENTS python${OPENEXR_PYTHON_MAJOR}${OPENEXR_PYTHON_MINOR})
    if(NOT Boost_PYTHON${OPENEXR_PYTHON_MAJOR}${OPENEXR_PYTHON_MINOR}_FOUND)
      message(WARNING "user provided Boost Python${OPENEXR_PYTHON_MAJOR}${OPENEXR_PYTHON_MINOR} was not found. Trying default path...")
      set(FIND_DEFAULT_PYTHON TRUE)
    endif()

  else()
    # older Boost versions < 1.67
    if(OPENEXR_PYTHON_MAJOR VERSION_GREATER_EQUAL 3)
      find_package(Boost COMPONENTS python${OPENEXR_PYTHON_MAJOR})
      if(NOT Boost_PYTHON${OPENEXR_PYTHON_MAJOR}_FOUND)
        message(WARNING "user provided Boost Python${OPENEXR_PYTHON_MAJOR} was not found. Trying default path...")
        set(FIND_DEFAULT_PYTHON TRUE)
      endif()
    else()
      set(FIND_DEFAULT_PYTHON TRUE)
    endif()
  endif()

  if(FIND_DEFAULT_PYTHON)
    find_package(Boost COMPONENTS python)
    if(NOT Boost_PYTHON_FOUND)
      message(WARNING "Make boost Python available to CMake's search path, and re-run configuration")
      message(FATAL_ERROR "boost Python is a required dependency when OPENEXR_BUILD_PYTHON_LIBS is set")
    endif()
  endif()

  find_package(NumPy)
endif()

if(OPENEXR_BUILD_VIEWERS)
  message(WARNING "Viewers are currently out of order. Building anyway")
  SET (FLTK_SKIP_FLUID 1)
  find_package(FLTK)
  if(NOT FLTK_FOUND)
    message(WARNING "FLTK not found, exrdisplay will not be built")
  endif()
  find_package(OpenGL)
  if(NOT OpenGL_FOUND)
    message(WARNING "OpenGL not found, exrdisplay will not be built")
  endif()
endif()

#]==]
