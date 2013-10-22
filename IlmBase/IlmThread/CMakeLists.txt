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

ADD_LIBRARY ( IlmThread
  ${ILMTHREAD_LIBRARY_SOURCES}
  )

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DILMTHREAD_EXPORTS)
  SET_TARGET_PROPERTIES(IlmThread PROPERTIES OUTPUT_NAME "IlmThread-2_0")
ENDIF()

TARGET_LINK_LIBRARIES(IlmThread Iex)

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
