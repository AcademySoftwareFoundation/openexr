
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

SET_ILMBASE_INCLUDE_DIRS(exrdisplay)

TARGET_INCLUDE_DIRECTORIES ( exrdisplay 
    PRIVATE
    ${CMAKE_INSTALL_PREFIX}/include/OpenEXR
  	${FLTK_INCLUDE_DIR} )

TARGET_LINK_LIBRARIES ( exrdisplay
	OpenEXR::IlmImf${OPENEXR_TARGET_SUFFIX}
	IlmBase::Iex${OPENEXR_TARGET_SUFFIX}
	IlmBase::Half${OPENEXR_TARGET_SUFFIX}
	${FLTK_LIBRARIES}
	${OPENGL_LIBRARIES}
)

INSTALL ( TARGETS
	exrdisplay
	DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
