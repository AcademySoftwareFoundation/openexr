
PROJECT (pyilmbase)

IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
  IF (WIN32)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
  ENDIF ()
ENDIF (BUILD_SHARED_LIBS)

LINK_DIRECTORIES ( ${ILMBASE_PACKAGE_PREFIX}/lib )
MESSAGE ( "ILMBASE_PACKAGE_PREFIX = " ${ILMBASE_PACKAGE_PREFIX})

##########################
ADD_SUBDIRECTORY ( PyIex )
ADD_SUBDIRECTORY ( PyImath )

IF (NUMPY_FOUND)
  ADD_SUBDIRECTORY ( PyImathNumpy )
  ADD_SUBDIRECTORY ( PyImathNumpyTest )
ENDIF ()

##########################
# Tests
##########################
ADD_SUBDIRECTORY ( PyIexTest )
ADD_SUBDIRECTORY ( PyImathTest )
