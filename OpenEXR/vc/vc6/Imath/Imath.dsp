# Microsoft Developer Studio Project File - Name="Imath" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Imath - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Imath.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Imath.mak" CFG="Imath - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Imath - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Imath - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "Imath - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PLATFORM_WIN32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Imath\Imath.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=post-build
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Imath - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../../../IlmImf" /I "../../../Iex" /I "../../../Imath" /I "../../../Half" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "PLATFORM_WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\Imath\Imathd.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=post-build
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Imath - Win32 Release"
# Name "Imath - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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
# End Target
# End Project
