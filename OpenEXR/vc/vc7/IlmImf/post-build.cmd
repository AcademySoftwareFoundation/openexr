set srcdir=..\..\..\IlmImf
cd %srcdir%
set instdir=..\vc\vc7\include\IlmImf
mkdir %instdir%
copy ImfAttribute.h %instdir%
copy ImfBoxAttribute.h %instdir%
copy ImfCRgbaFile.h %instdir%
copy ImfChannelList.h %instdir%
copy ImfChannelListAttribute.h %instdir%
copy ImfCompressionAttribute.h %instdir%
copy ImfDoubleAttribute.h %instdir%
copy ImfFloatAttribute.h %instdir%
copy ImfFrameBuffer.h %instdir%
copy ImfHeader.h %instdir%
copy ImfIO.h %instdir%
copy ImfInputFile.h %instdir%
copy ImfIntAttribute.h %instdir%
copy ImfLineOrderAttribute.h %instdir%
copy ImfMatrixAttribute.h %instdir%
copy ImfOpaqueAttribute.h %instdir%
copy ImfOutputFile.h %instdir%
copy ImfRgbaFile.h %instdir%
copy ImfStringAttribute.h %instdir%
copy ImfVecAttribute.h %instdir%
copy ImfHuf.h %instdir%
copy ImfWav.h %instdir%
copy ImfLut.h %instdir%
copy ImfArray.h %instdir%
copy ImfCompression.h %instdir%
copy ImfLineOrder.h %instdir%
copy ImfName.h %instdir%
copy ImfPixelType.h %instdir%
copy ImfVersion.h %instdir%
copy ImfXdr.h %instdir%
copy ImfConvert.h %instdir%
copy ImfPreviewImage.h %instdir%
copy ImfPreviewImageAttribute.h %instdir%
copy ImfChromaticities.h %instdir%
copy ImfChromaticitiesAttribute.h %instdir%
copy ImfStandardAttributes.h %instdir%
copy ImfStdIO.h  %instdir%
copy ImfEnvmap.h  %instdir%
copy ImfEnvmapAttribute.h  %instdir%
copy ImfInt64.h  %instdir%
copy ImfRgba.h %instdir%     
copy ImfTileDescription.h  %instdir%
copy ImfTileDescriptionAttribute.h  %instdir%
copy ImfTiledInputFile.h  %instdir%
copy ImfTiledOutputFile.h  %instdir%
copy ImfTiledRgbaFile.h %instdir%
copy ImfRgbaYca.h %instdir%
		       
mkdir ..\vc\vc7\lib\zlib
copy ..\..\zlib\zlib.lib ..\vc\vc7\lib\zlib
copy ..\..\zlib\zlibd.lib ..\vc\vc7\lib\zlib
