# Microsoft Developer Studio Project File - Name="IlmImfDll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IlmImfDll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IlmImfDll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IlmImfDll.mak" CFG="IlmImfDll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IlmImfDll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IlmImfDll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IlmImfDll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /I "../../../../zlib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PLATFORM_WIN32" /D "IMF_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 zlib_static.lib kernel32.lib user32.lib gdi32.lib winspool.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../lib/IlmImfdll/IlmImf.dll" /implib:"../lib/IlmImfdll/IlmImfdll.lib" /libpath:"../../../../zlib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "IlmImfDll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /I "../../../../zlib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "PLATFORM_WIN32" /D "IMF_DLL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlibd_static.lib kernel32.lib user32.lib gdi32.lib winspool.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../lib/IlmImfdll/IlmImfd.dll" /implib:"../lib/IlmImfdll/IlmImfdlld.lib" /pdbtype:sept /libpath:"../../../../zlib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "IlmImfDll - Win32 Release"
# Name "IlmImfDll - Win32 Debug"
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
# Begin Group "Imath"

# PROP Default_Filter ""
# Begin Group "Source Files No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Imath\ImathColorAlgo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathFun.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathMatrixAlgo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathShear.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathVec.cpp
# End Source File
# End Group
# Begin Group "Header Files No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Imath\ImathBox.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathBoxAlgo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathColor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathColorAlgo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathEuler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathExc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathFrame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathFrustum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathFun.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathGL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathGLU.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathHalfLimits.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathInterval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathLimits.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathLine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathLineAlgo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathMath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathMatrixAlgo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathPlatform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathQuat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathRandom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathRoots.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathShear.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathSphere.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathVec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Imath\ImathVecAlgo.h
# End Source File
# End Group
# End Group
# Begin Group "Iex"

# PROP Default_Filter ""
# Begin Group "Source Files No. 2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Iex\IexBaseExc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexThrowErrnoExc.cpp
# End Source File
# End Group
# Begin Group "Header Files No. 2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Iex\Iex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexBaseExc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexErrnoExc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexMathExc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Iex\IexThrowErrnoExc.h
# End Source File
# End Group
# End Group
# Begin Group "Half"

# PROP Default_Filter ""
# Begin Group "Source Files No. 3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Half\half.cpp
# End Source File
# End Group
# Begin Group "Header Files No. 3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Half\half.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Half\halfFunction.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Half\halfLimits.h
# End Source File
# End Group
# End Group
# End Target
# End Project
