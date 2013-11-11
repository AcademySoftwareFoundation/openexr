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
        IlmImf
        Half 
        Iex-2_1
        Imath-2_1 
        IlmThread-2_1 
        ${PTHREAD_LIB} ${ZLIB_LIBRARIES}
        )
  