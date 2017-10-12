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


IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DILMTHREAD_EXPORTS)
ENDIF()

ADD_LIBRARY ( IlmThread ${LIB_TYPE}
  ${ILMTHREAD_LIBRARY_SOURCES}
  )
TARGET_LINK_LIBRARIES(IlmThread Iex)

IF (OPENEXR_INSTALL)

  INSTALL ( TARGETS
    IlmThread
    DESTINATION
    lib
  )

  INSTALL ( FILES 
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

ENDIF(OPENEXR_INSTALL)
