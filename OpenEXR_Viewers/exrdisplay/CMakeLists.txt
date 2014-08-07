
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
	${FLTK_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES ( exrdisplay
	IlmImf${OPENEXR_LIBSUFFIX}
	Iex${ILMBASE_LIBSUFFIX}
	Half
	${FLTK_LIBRARIES}
	${OPENGL_LIBRARIES}
)

INSTALL ( TARGETS
	exrdisplay
	DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
