# Microsoft Developer Studio Project File - Name="IlmImfTest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=IlmImfTest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IlmImfTest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IlmImfTest.mak" CFG="IlmImfTest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IlmImfTest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "IlmImfTest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "IlmImfTest - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../IlmImfTest" /I "../../../Half" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:console /machine:I386 /libpath:"..\..\..\..\zlib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running IlmImfTest program...
PostBuild_Cmds=copy ..\..\..\IlmImfTest\*.exr .	set path=%path%;..\..\..\..\zlib\lib	Release\IlmImfTest.exe	del *.exr
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IlmImfTest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../IlmImfTest" /I "../../../Half" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlibd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\zlib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running IlmImfTest program...
PostBuild_Cmds=copy ..\..\..\IlmImfTest\*.exr .	set path=%path%;..\..\..\..\zlib\lib	Debug\IlmImfTest.exe	del *.exr
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "IlmImfTest - Win32 Release"
# Name "IlmImfTest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\IlmImfTest\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testAttributes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testChannels.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCompression.cpp
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testConversion.cpp"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCopyPixels.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCustomAttributes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testHuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testLineOrder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testLut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testMagic.cpp
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testNativeFormat.cpp"
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testPreviewImage.cpp"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testRgba.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testSampleImages.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testStandardAttributes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testWav.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testXdr.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testAttributes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testChannels.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCompression.h
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testConversion.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCopyPixels.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testCustomAttributes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testHuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testLineOrder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testLut.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testMagic.h
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testNativeFormat.h"
# End Source File
# Begin Source File

SOURCE="\tmp\OpenEXR-1.0.6\IlmImfTest\testPreviewImage.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testRgba.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testSampleImages.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testStandardAttributes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testWav.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IlmImfTest\testXdr.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
