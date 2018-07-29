# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IlmImfExamples
  drawImage.cpp
  generalInterfaceExamples.cpp
  generalInterfaceTiledExamples.cpp
  lowLevelIoExamples.cpp
  main.cpp
  previewImageExamples.cpp
  rgbaInterfaceExamples.cpp
  rgbaInterfaceTiledExamples.cpp
 )


TARGET_LINK_LIBRARIES ( IlmImfExamples 
        OpenEXR::IlmImf
        IlmBase::Half
        IlmBase::Iex
        IlmBase::Imath
        IlmBase::IlmThread
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
        )
  
