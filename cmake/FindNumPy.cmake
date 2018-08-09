

if(NOT PYTHON_EXECUTABLE)
  find_package(PythonInterp QUIET)
endif()

if (PYTHON_EXECUTABLE)
  # Find out the include path
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
    "from __future__ import print_function\ntry: import numpy; print(numpy.get_include(), end='')\nexcept:pass\n"
    RESULT_VARIABLE _NUMPY_RESULT
    OUTPUT_VARIABLE py_ilmbase_numpy_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT _NUMPY_RESULT MATCHES 0)
    set(NumPy_FOUND 0 CACHE INTERNAL "Python numpy not found")
    return()
  endif()

  # And the version
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
    "from __future__ import print_function\ntry: import numpy; print(numpy.__version__, end='')\nexcept:pass\n"
    RESULT_VARIABLE _NUMPY_RESULT
    OUTPUT_VARIABLE py_ilmbase_numpy_version
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT _NUMPY_RESULT MATCHES 0)
    set(NumPy_FOUND 0 CACHE INTERNAL "Python numpy not found")
    return()
  endif()

  find_path(NUMPY_INCLUDE_DIRS numpy/arrayobject.h
    HINTS "${py_ilmbase_numpy_path}" "${PYTHON_INCLUDE_PATH}" NO_DEFAULT_PATH)

  if(NUMPY_INCLUDE_DIRS)
    set(NumPy_FOUND 1 CACHE INTERNAL "Python numpy found")

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(NumPy REQUIRED_VARS NUMPY_INCLUDE_DIRS
      VERSION_VAR py_ilmbase_numpy_version)
  else()
    set(NumPy_FOUND 0 CACHE INTERNAL "Python numpy not found")
    message(WARNING "Numpy not found, PyImathNumpy will not be built")
  endif()

else ()
  message(WARNING "Numpy not found, PyImathNumpy will not be built")
  set(NumPy_FOUND 0 CACHE INTERNAL "Python numpy not found")
endif()
