# yue.nicholas@gmail.com

SET( ILMTHREAD_LIBRARY_SOURCES
  IlmThread.cpp
  IlmThreadMutex.cpp
  IlmThreadMutexPosix.cpp
  IlmThreadPool.cpp
  IlmThreadPosix.cpp
  IlmThreadSemaphore.cpp
  IlmThreadSemaphorePosixCompat.cpp
  IlmThreadSemaphorePosix.cpp
)
IF (WIN32)
  SET( ILMTHREAD_LIBRARY_SOURCES ${ILMTHREAD_LIBRARY_SOURCES}
    IlmThreadMutexWin32.cpp
    IlmThreadSemaphoreWin32.cpp
    IlmThreadWin32.cpp
  )
ENDIF()

SET (ILMBASE_LIB_TARGETS "")

IF ( OPENEXR_BUILD_SHARED )
  LIST ( APPEND ILMBASE_LIB_TARGETS IlmThread )

  ADD_LIBRARY ( IlmThread SHARED ${ILMTHREAD_LIBRARY_SOURCES} )

  TARGET_COMPILE_DEFINITIONS ( IlmThread PRIVATE ILMTHREAD_EXPORTS )
  IF (WIN32)
    TARGET_COMPILE_DEFINITIONS ( IlmThread PUBLIC OPENEXR_DLL )
  ENDIF ()

  TARGET_LINK_LIBRARIES( IlmThread
    PUBLIC IlmBase::Iex )

  SET_TARGET_PROPERTIES ( IlmThread
    PROPERTIES
    VERSION ${OPENEXR_VERSION}
    SOVERSION ${OPENEXR_SOVERSION}
    OUTPUT_NAME "IlmThread${ILMBASE_LIBSUFFIX}"
    )
ENDIF ()

IF (BUILD_ILMBASE_STATIC)
  LIST ( APPEND ILMBASE_LIB_TARGETS IlmThread_static )

  ADD_LIBRARY ( IlmThread_static STATIC ${ILMTHREAD_LIBRARY_SOURCES} )

  SET_TARGET_PROPERTIES ( IlmThread_static
    PROPERTIES
    VERSION ${ILMBASE_VERSION_MAJOR}.${ILMBASE_VERSION_MINOR}.${ILMBASE_VERSION_PATCH}
    OUTPUT_NAME "IlmThread${ILMBASE_LIBSUFFIX}_s"
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
    IlmThreadPool.h
    IlmThread.h
    IlmThreadSemaphore.h
    IlmThreadMutex.h
    IlmThreadNamespace.h
    IlmThreadExport.h
    IlmThreadForward.h

  DESTINATION
    include/OpenEXR
)

if (OPENEXR_BUILD_SHARED)
    add_library(IlmBase::IlmThread ALIAS IlmThread)
endif()
if (BUILD_ILMBASE_STATIC)
    add_library(IlmBase::IlmThread_static ALIAS IlmThread_static)
endif()
