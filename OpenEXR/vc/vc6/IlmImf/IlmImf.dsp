# Microsoft Developer Studio Project File - Name="IlmImf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=IlmImf - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IlmImf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IlmImf.mak" CFG="IlmImf - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IlmImf - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "IlmImf - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "IlmImf - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /I "../../../../zlib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PLATFORM_WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\IlmImf\IlmImf.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=post-build
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IlmImf - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /I "../../../../zlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "PLATFORM_WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\IlmImf\IlmImfd.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=post-build
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "IlmImf - Win32 Release"
# Name "IlmImf - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfBoxAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChannelList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChannelListAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChromaticities.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChromaticitiesAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCompressionAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCompressor.cpp
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfConvert.cpp"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCRgbaFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfDoubleAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfFloatAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfFrameBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfHuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfInputFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfIntAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfLineOrderAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfLut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfMatrixAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfMisc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfOpaqueAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfOutputFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfPizCompressor.cpp
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfPreviewImage.cpp"
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfPreviewImageAttribute.cpp"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfRgbaFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfRleCompressor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfStandardAttributes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfStringAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfVecAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfVersion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfWav.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfZipCompressor.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfArray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfAutoArray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfBoxAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChannelList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChannelListAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChromaticities.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfChromaticitiesAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCompression.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCompressionAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCompressor.h
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfConvert.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfCRgbaFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfDoubleAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfFloatAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfFrameBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfHuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfInputFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfIntAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfLineOrder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfLineOrderAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfLut.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfMatrixAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfMisc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfOpaqueAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfOutputFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfPixelType.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfPizCompressor.h
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfPreviewImage.h"
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImf\ImfPreviewImageAttribute.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfRgbaFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfRleCompressor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfStandardAttributes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfStringAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfVecAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfVersion.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfWav.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfXdr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImf\ImfZipCompressor.h
# End Source File
# End Group
# End Target
# End Project
