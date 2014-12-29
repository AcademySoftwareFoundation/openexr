
ADD_EXECUTABLE ( exrdisplay
	main.cpp 
	ImageView.cpp
	ImageView.h
	loadImage.cpp
	loadImage.h
	scaleImage.cpp
	scaleImage.h
	applyCtl.cpp
	applyCtl.h
	GlWindow3d.h
	GlWindow3d.cpp
)

INCLUDE_DIRECTORIES ( 
    ${IlmBase_INCLUDE_DIR}
    ${OpenEXR_INCLUDE_DIR}
	${FLTK_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES ( exrdisplay
	${IlmBase_LIBRARY}
	${OpenEXR_LIBRARY}
	${FLTK_LIBRARIES}
	${OPENGL_LIBRARIES}
)

INSTALL ( TARGETS
	exrdisplay
	DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
