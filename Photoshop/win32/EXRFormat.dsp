# Microsoft Developer Studio Project File - Name="SimpleFormat" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SimpleFormat - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EXRFormat.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EXRFormat.mak" CFG="SimpleFormat - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SimpleFormat - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SimpleFormat - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "SimpleFormat"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SimpleFormat - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\temp"
# PROP Intermediate_Dir ".\temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "ISOLATION_AWARE_ENABLED" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\rsrc" /I "..\src\framework" /I "..\src\main" /I "..\src\resample" /I "..\src\ui" /I "..\sdk\PhotoshopAPI\ADM" /I "..\sdk\PhotoshopAPI\General" /I "..\sdk\PhotoshopAPI\Photoshop" /I "..\sdk\PhotoshopAPI\Pica_sp" /I "..\sdk\PhotoshopAPI\Resources" /I "..\sdk\PhotoshopUtils\Includes" /I "..\..\OpenEXR\Half" /I "..\..\OpenEXR\Iex" /I "..\..\OpenEXR\Imath" /I "..\..\OpenEXR\IlmImf" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D MSWindows=1 /D "ISOLATION_AWARE_ENABLED" /D "PLATFORM_WIN32" /D "WIN_ENV" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d MSWindows=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug\EXRFormat.8bi" /libpath:"..\Common" /libpath:"..\..\..\Common\Headers\Photoshop" /libpath:"..\..\..\Common\Headers\Photoshop\ADM" /libpath:"..\..\..\Common\Headers\Photoshop\AS" /libpath:"..\..\..\Common\Headers\Photoshop\PICA" /libpath:"..\..\..\Common\Headers\Photoshop\PS-Suites" /libpath:"..\..\..\Common\Headers\SDK" /libpath:"..\..\..\Common\Rez-files\Photoshop" /libpath:"..\..\..\Common\Rez-files\SDK"
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "SimpleFormat - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "SimpleFormat___Win32_Release"
# PROP BASE Intermediate_Dir "SimpleFormat___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "temp"
# PROP Intermediate_Dir "temp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\rsrc" /I "..\src\framework" /I "..\src\main" /I "..\src\resample" /I "..\src\ui" /I "..\sdk\PhotoshopAPI\ADM" /I "..\sdk\PhotoshopAPI\General" /I "..\sdk\PhotoshopAPI\Photoshop" /I "..\sdk\PhotoshopAPI\Pica_sp" /I "..\sdk\PhotoshopAPI\Resources" /I "..\sdk\PhotoshopUtils\Includes" /I "..\..\OpenEXR\Half" /I "..\..\OpenEXR\Iex" /I "..\..\OpenEXR\Imath" /I "..\..\OpenEXR\IlmImf" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D MSWindows=1 /D "ISOLATION_AWARE_ENABLED" /D "PLATFORM_WIN32" /D "WIN_ENV" /FR /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /O2 /I "..\rsrc" /I "..\src\framework" /I "..\src\main" /I "..\src\resample" /I "..\src\ui" /I "..\sdk\PhotoshopAPI\ADM" /I "..\sdk\PhotoshopAPI\General" /I "..\sdk\PhotoshopAPI\Photoshop" /I "..\sdk\PhotoshopAPI\Pica_sp" /I "..\sdk\PhotoshopAPI\Resources" /I "..\sdk\PhotoshopUtils\Includes" /I "..\..\OpenEXR\Half" /I "..\..\OpenEXR\Iex" /I "..\..\OpenEXR\Imath" /I "..\..\OpenEXR\IlmImf" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D MSWindows=1 /D "ISOLATION_AWARE_ENABLED" /D "PLATFORM_WIN32" /D "WIN_ENV" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d MSWindows=1
# ADD RSC /l 0x409 /d "_DEBUG" /d MSWindows=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug\EXRFormat.8bi" /libpath:"..\Common" /libpath:"..\..\..\Common\Headers\Photoshop" /libpath:"..\..\..\Common\Headers\Photoshop\ADM" /libpath:"..\..\..\Common\Headers\Photoshop\AS" /libpath:"..\..\..\Common\Headers\Photoshop\PICA" /libpath:"..\..\..\Common\Headers\Photoshop\PS-Suites" /libpath:"..\..\..\Common\Headers\SDK" /libpath:"..\..\..\Common\Rez-files\Photoshop" /libpath:"..\..\..\Common\Rez-files\SDK"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release\EXRFormat.8bi" /libpath:"..\Common" /libpath:"..\..\..\Common\Headers\Photoshop" /libpath:"..\..\..\Common\Headers\Photoshop\ADM" /libpath:"..\..\..\Common\Headers\Photoshop\AS" /libpath:"..\..\..\Common\Headers\Photoshop\PICA" /libpath:"..\..\..\Common\Headers\Photoshop\PS-Suites" /libpath:"..\..\..\Common\Headers\SDK" /libpath:"..\..\..\Common\Rez-files\Photoshop" /libpath:"..\..\..\Common\Rez-files\SDK"
# SUBTRACT LINK32 /incremental:no /debug

!ENDIF 

# Begin Target

# Name "SimpleFormat - Win32 Debug"
# Name "SimpleFormat - Win32 Release"
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\main\ExrFormatGlobals.cpp
# End Source File
# Begin Source File

SOURCE=..\src\main\EXRFormatGlobals.h
# End Source File
# Begin Source File

SOURCE=..\src\main\EXRFormatPlugin.cpp
# End Source File
# Begin Source File

SOURCE=..\src\main\EXRFormatPlugin.h
# End Source File
# Begin Source File

SOURCE=..\src\resample\EXRResample.cpp
# End Source File
# Begin Source File

SOURCE=..\src\resample\EXRResample.h
# End Source File
# Begin Source File

SOURCE=..\src\main\FSSpecToFullPath.cpp
# End Source File
# Begin Source File

SOURCE=..\src\main\FSSpecToFullPath.h
# End Source File
# End Group
# Begin Group "Framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\framework\PSAutoBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\framework\PSAutoBuffer.h
# End Source File
# Begin Source File

SOURCE=..\src\framework\PSFormatGlobals.h
# End Source File
# Begin Source File

SOURCE=..\src\framework\PSFormatPlugin.cpp
# End Source File
# Begin Source File

SOURCE=..\src\framework\PSFormatPlugin.h
# End Source File
# End Group
# Begin Group "UI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\ui\EXRExportDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ui\EXRExportDialog.h
# End Source File
# Begin Source File

SOURCE=..\src\ui\EXRImportDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ui\EXRImportDialog.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\rsrc\EXRFormat.r

!IF  "$(CFG)" == "SimpleFormat - Win32 Debug"

# Begin Custom Build - Compiling PiPL resource...
IntDir=.\temp
InputPath=..\rsrc\EXRFormat.r
InputName=EXRFormat

"$(IntDir)\$(InputName).pipl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /I..\sdk\PhotoshopAPI\General  /I..\sdk\PhotoshopAPI\PICA_SP /I..\sdk\PhotoshopAPI\Photoshop   /I..\sdk\PhotoshopAPI\Resources  /EP /DMSWindows=1 /Tc$(InputPath) >     $(IntDir)\$(InputName).rr 
	..\sdk\resources\cnvtpipl $(IntDir)\$(InputName).rr $(IntDir)\$(InputName).pipl 
	del $(IntDir)\$(InputName).rr 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "SimpleFormat - Win32 Release"

# Begin Custom Build - Compiling PiPL resource...
IntDir=.\temp
InputPath=..\rsrc\EXRFormat.r
InputName=EXRFormat

"$(IntDir)\$(InputName).pipl" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /I..\sdk\PhotoshopAPI\General  /I..\sdk\PhotoshopAPI\PICA_SP /I..\sdk\PhotoshopAPI\Photoshop   /I..\sdk\PhotoshopAPI\Resources  /EP /DMSWindows=1 /Tc$(InputPath) >     $(IntDir)\$(InputName).rr 
	..\sdk\resources\cnvtpipl $(IntDir)\$(InputName).rr $(IntDir)\$(InputName).pipl 
	del $(IntDir)\$(InputName).rr 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EXRFormat.rc
# End Source File
# End Group
# Begin Group "EXR Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OpenEXR\vc\vc6\lib\Half\Halfd.lib
# End Source File
# Begin Source File

SOURCE=..\..\OpenEXR\vc\vc6\lib\Iex\Iexd.lib
# End Source File
# Begin Source File

SOURCE=..\..\OpenEXR\vc\vc6\lib\IlmImf\IlmImfd.lib
# End Source File
# Begin Source File

SOURCE=..\..\OpenEXR\vc\vc6\lib\Imath\Imathd.lib
# End Source File
# Begin Source File

SOURCE=..\..\zlib\zlib\Debug\zlib.lib
# End Source File
# End Group
# End Target
# End Project
