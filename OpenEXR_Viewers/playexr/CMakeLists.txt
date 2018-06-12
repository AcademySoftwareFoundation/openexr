
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
	${IlmBase_INCLUDE_DIR}
	${OpenEXR_INCLUDE_DIR}
	${CG_INCLUDE_PATH}
)

message( "IlmBase_INCLUDE_DIR: ${IlmBase_INCLUDE_DIR}")

TARGET_LINK_LIBRARIES ( playexr
	${IlmBase_LIBRARIES}
	${OpenEXR_LIBRARIES}
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

