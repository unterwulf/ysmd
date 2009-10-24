# Microsoft Developer Studio Project File - Name="ysm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ysm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ysm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ysm.mak" CFG="ysm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ysm - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ysm - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ysm - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib ws2_32.lib advapi32.lib winmm.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "ysm - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ysm - Win32 Release"
# Name "ysm - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "rijndael"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=".\src\rijndael\rijndael-alg-fst.c"
# End Source File
# Begin Source File

SOURCE=".\src\rijndael\rijndael-api-fst.c"
# End Source File
# End Group
# Begin Group "cmdline"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cmdline\getline.c
# End Source File
# Begin Source File

SOURCE=.\src\cmdline\ysmline.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\YSM_Charset.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Commands.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Crypt.c
# ADD CPP /I "src/rijndael/"
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Direct.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_FishGUI.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Lists.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Main.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Network.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Prompt.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Setup.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Slaves.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_ToolBox.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Win32.c
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Wrappers.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "YSMLangs"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\src\YSMLangs\english.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\french_1.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\french_2.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\german.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\italian.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\portuguesebr.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\russian.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\spanish.h
# End Source File
# Begin Source File

SOURCE=.\src\YSMLangs\swedish.h
# End Source File
# End Group
# Begin Source File

SOURCE=".\src\rijndael\rijndael-alg-fst.h"
# End Source File
# Begin Source File

SOURCE=".\src\rijndael\rijndael-api-fst.h"
# End Source File
# Begin Source File

SOURCE=.\src\YSM.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Charset.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Commands.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Config.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Crypt.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Direct.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_FishGUI.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_ICQv7.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Lang.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Lists.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Main.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Network.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Prompt.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Setup.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Slaves.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_ToolBox.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Win32.h
# End Source File
# Begin Source File

SOURCE=.\src\YSM_Wrappers.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
