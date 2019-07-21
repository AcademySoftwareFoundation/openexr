# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_executable(exrdisplay
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
target_include_directories(exrdisplay PRIVATE ${FLTK_INCLUDE_DIR})
target_link_libraries(exrdisplay
  ${FLTK_LIBRARIES}
  OpenGL::GL
  OpenEXR::IlmImf
)

install(TARGETS exrdisplay DESTINATION ${CMAKE_INSTALL_BINDIR})
