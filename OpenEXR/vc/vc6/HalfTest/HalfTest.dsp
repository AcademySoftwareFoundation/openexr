# Microsoft Developer Studio Project File - Name="HalfTest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=HalfTest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HalfTest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HalfTest.mak" CFG="HalfTest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HalfTest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "HalfTest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HalfTest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../../Half" /I "../../../HalfTest" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:console /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running HalfTest program....
PostBuild_Cmds=Release\HalfTest.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "HalfTest - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../../../Half" /I "../../../HalfTest" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "PLATFORM_WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Running HalfTest program....
PostBuild_Cmds=Debug\HalfTest.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "HalfTest - Win32 Release"
# Name "HalfTest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\main.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testArithmetic.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testBitPatterns.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testClassification.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testError.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testFunction.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testLimits.cpp
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testSize.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testArithmetic.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testBitPatterns.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testClassification.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testError.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testFunction.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testLimits.h
# End Source File
# Begin Source File

SOURCE=\src\OpenEXR\HalfTest\testSize.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
