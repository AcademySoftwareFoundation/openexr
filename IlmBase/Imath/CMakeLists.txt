# yue.nicholas@gmail.com

SET (ILMBASE_LIB_TARGETS "")

SET ( IMATH_SOURCES
  ImathRandom.cpp
  ImathColorAlgo.cpp
  ImathFun.cpp
  ImathVec.cpp
  ImathMatrixAlgo.cpp
)

IF ( OPENEXR_BUILD_SHARED )
  LIST ( APPEND ILMBASE_LIB_TARGETS Imath )

  ADD_LIBRARY ( Imath SHARED ${IMATH_SOURCES} )

  TARGET_COMPILE_DEFINITIONS ( Imath PRIVATE IMATH_EXPORTS )
  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( Imath PUBLIC OPENEXR_DLL )
  ENDIF ()

  TARGET_LINK_LIBRARIES(Imath PUBLIC IlmBase::Iex)
  SET_TARGET_PROPERTIES ( Imath
    PROPERTIES
    VERSION ${OPENEXR_VERSION}
    SOVERSION ${OPENEXR_SOVERSION}
    OUTPUT_NAME "Imath${ILMBASE_LIBSUFFIX}"
    )
ENDIF ()

IF (BUILD_ILMBASE_STATIC)
  LIST ( APPEND ILMBASE_LIB_TARGETS Imath_static )

  ADD_LIBRARY ( Imath_static STATIC ${IMATH_SOURCES} )

  SET_TARGET_PROPERTIES ( Imath_static
    PROPERTIES
    VERSION ${ILMBASE_VERSION_MAJOR}.${ILMBASE_VERSION_MINOR}.${ILMBASE_VERSION_PATCH}
    OUTPUT_NAME "Imath${ILMBASE_LIBSUFFIX}_s"
    )
ENDIF ()

IF (OPENEXR_BUILD_SHARED OR BUILD_ILMBASE_STATIC)
  INSTALL ( TARGETS ${ILMBASE_LIB_TARGETS}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION ${RUNTIME_DIR}
  )
ENDIF ()

INSTALL (
  FILES
    ImathBoxAlgo.h
    ImathBox.h
    ImathColorAlgo.h
    ImathColor.h
    ImathEuler.h
    ImathExc.h
    ImathExport.h
    ImathForward.h
    ImathFrame.h
    ImathFrustum.h
    ImathFrustumTest.h
    ImathFun.h
    ImathGL.h
    ImathGLU.h
    ImathHalfLimits.h
    ImathInt64.h
    ImathInterval.h
    ImathLimits.h
    ImathLineAlgo.h
    ImathLine.h
    ImathMath.h
    ImathMatrixAlgo.h
    ImathMatrix.h
    ImathNamespace.h
    ImathPlane.h
    ImathPlatform.h
    ImathQuat.h
    ImathRandom.h
    ImathRoots.h
    ImathShear.h
    ImathSphere.h
    ImathVecAlgo.h
    ImathVec.h

  DESTINATION
    include/OpenEXR
)

if (OPENEXR_BUILD_SHARED)
    add_library(IlmBase::Imath ALIAS Imath)
endif()
if (BUILD_ILMBASE_STATIC)
    add_library(IlmBase::Imath_static ALIAS Imath_static)
endif()
