# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrenvmap
  makeLatLongMap.cpp
  readInputImage.cpp
  resizeImage.cpp
  makeCubeMap.cpp
  main.cpp
  blurImage.cpp
  EnvmapImage.cpp
)

TARGET_LINK_LIBRARIES ( exrenvmap
  IlmImf
  ${PTHREAD_LIB}
)
TARGET_INCLUDE_DIRECTORIES ( exrenvmap
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:IlmImf,INTERFACE_INCLUDE_DIRS>
  )

INSTALL ( TARGETS
  exrenvmap
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
