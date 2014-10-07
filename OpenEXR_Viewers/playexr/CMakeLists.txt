
ADD_EXECUTABLE ( playexr
	main.cpp 
	playExr.cpp
	playExr.h
	fileNameForFrame.cpp
	fileNameForFrame.h
	FileReadingThread.cpp
	FileReadingThread.h
	ImageBuffers.cpp
	ImageBuffers.h
	Timer.cpp
	Timer.h
	ctlToLut.cpp
	ctlToLut.h
	osDependent.cpp
	osDependent.h 
)

INCLUDE_DIRECTORIES ( 
	${CG_INCLUDE_PATH}
)

TARGET_LINK_LIBRARIES ( playexr
	IlmImf${OPENEXR_LIBSUFFIX}
	Iex${ILMBASE_LIBSUFFIX}
	IlmThread${ILMBASE_LIBSUFFIX}
	Half
	${CG_LIBRARY}
	${CG_GL_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLUT_LIBRARY}
)

INSTALL ( TARGETS
	playexr
	DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)

