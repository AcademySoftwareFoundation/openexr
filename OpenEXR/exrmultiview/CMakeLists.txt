# yue.nicholas@gmail.com

ADD_EXECUTABLE ( exrmultiview
  makeMultiView.cpp
  main.cpp
  Image.cpp
)

TARGET_LINK_LIBRARIES ( exrmultiview
  IlmImf
  ${PTHREAD_LIB}
  ${ZLIB_LIBRARIES}
)
TARGET_INCLUDE_DIRECTORIES ( exrmultiview
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:IlmImf,INTERFACE_INCLUDE_DIRS>
  )

INSTALL ( TARGETS
  exrmultiview
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/bin
)
