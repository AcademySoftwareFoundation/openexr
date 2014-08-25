# - Try to find NumPy includes
#
# Uses find_package(PythonInterp), you can customize PYTHON_EXECUTABLE if
# it picks the wrong python executable.
#
# Sets following variables:
#    NUMPY_FOUND
#    NUMPY_INCLUDE_DIRS
#    NUMPY_VERSION
#

if(NumPy_FIND_REQUIRED)
  find_package(PythonInterp REQUIRED)
else()
  find_package(PythonInterp)
endif()

if(NOT PYTHONINTERP_FOUND)
  set(NUMPY_FOUND FALSE)
  return()
endif()

execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
  "import numpy as n; print(n.__version__);"
  RESULT_VARIABLE _NUMPY_SUCCESS
  OUTPUT_VARIABLE NUMPY_VERSION
  ERROR_VARIABLE _NUMPY_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT _NUMPY_SUCCESS MATCHES 0)
  if(NumPy_FIND_REQUIRED)
    message(FATAL_ERROR
      "import numpy failure:\n${_NUMPY_ERROR}")
  endif()
  set(NUMPY_FOUND FALSE)
  return()
endif()

execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
  "import numpy as n; print(n.get_include());"
  RESULT_VARIABLE _NUMPY_SUCCESS
  OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
  ERROR_VARIABLE _NUMPY_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT _NUMPY_SUCCESS MATCHES 0)
  if(NumPy_FIND_REQUIRED)
    message(FATAL_ERROR
      "import numpy failure:\n${_NUMPY_ERROR}")
  endif()
  set(NUMPY_FOUND FALSE)
  return()
endif()

unset(_NUMPY_SUCCESS)

set(NUMPY_INCLUDE_DIRS ${NUMPY_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(NumPy
    REQUIRED_VARS
        NUMPY_INCLUDE_DIR
    VERSION_VAR
        NUMPY_VERSION
)
