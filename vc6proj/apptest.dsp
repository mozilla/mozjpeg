# Microsoft Developer Studio Project File - Name="apptest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=apptest - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "apptest.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "apptest.mak" CFG="apptest - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "apptest - Win32 Release" ("Win32 (x86) Generic Project" 用)
!MESSAGE "apptest - Win32 Debug" ("Win32 (x86) Generic Project" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "apptest - Win32 Release"

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
# Begin Special Build Tool
OutDir=.\Release
SOURCE="$(InputPath)"
PostBuild_Cmds=fc /b .\testimg.ppm $(OutDir)\testout.ppm	fc /b .\testimg.bmp $(OutDir)\testout.bmp	fc /b .\testimg.jpg $(OutDir)\testout.jpg	fc /b .\testimg.ppm $(OutDir)\testoutp.ppm	fc /b .\testimgp.jpg $(OutDir)\testoutp.jpg	fc /b .\testorig.jpg $(OutDir)\testoutt.jpg
# End Special Build Tool

!ELSEIF  "$(CFG)" == "apptest - Win32 Debug"

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
# Begin Special Build Tool
OutDir=.\Debug
SOURCE="$(InputPath)"
PostBuild_Cmds=fc /b .\testimg.ppm $(OutDir)\testout.ppm	fc /b .\testimg.bmp $(OutDir)\testout.bmp	fc /b .\testimg.jpg $(OutDir)\testout.jpg	fc /b .\testimg.ppm $(OutDir)\testoutp.ppm	fc /b .\testimgp.jpg $(OutDir)\testoutp.jpg	fc /b .\testorig.jpg $(OutDir)\testoutt.jpg
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "apptest - Win32 Release"
# Name "apptest - Win32 Debug"
# Begin Group "Test Image Files"

# PROP Default_Filter "*.jpg;*.bmp;*.ppm"
# Begin Source File

SOURCE=.\testimg.bmp
# End Source File
# Begin Source File

SOURCE=.\testimg.jpg
# End Source File
# Begin Source File

SOURCE=.\testimg.ppm

!IF  "$(CFG)" == "apptest - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputDir=.
OutDir=.\Release
InputPath=.\testimg.ppm

BuildCmds= \
	echo $(OutDir)\cjpeg -dct int -outfile $(OutDir)\testout.jpg .\testimg.ppm \
	$(OutDir)\cjpeg -dct int -outfile $(OutDir)\testout.jpg .\testimg.ppm \
	echo $(OutDir)\cjpeg -dct int -progressive -opt -outfile $(OutDir)\testoutp.jpg .\testimg.ppm \
	$(OutDir)\cjpeg -dct int -progressive -opt -outfile $(OutDir)\testoutp.jpg .\testimg.ppm \
	

"$(OutDir)\testout.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testoutp.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "apptest - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Testing - $(InputPath)
InputDir=.
OutDir=.\Debug
InputPath=.\testimg.ppm

BuildCmds= \
	echo $(OutDir)\cjpeg -dct int -outfile $(OutDir)\testout.jpg .\testimg.ppm \
	$(OutDir)\cjpeg -dct int -outfile $(OutDir)\testout.jpg .\testimg.ppm \
	echo $(OutDir)\cjpeg -dct int -progressive -opt -outfile $(OutDir)\testoutp.jpg .\testimg.ppm \
	$(OutDir)\cjpeg -dct int -progressive -opt -outfile $(OutDir)\testoutp.jpg .\testimg.ppm \
	

"$(OutDir)\testout.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testoutp.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\testimgp.jpg
# End Source File
# Begin Source File

SOURCE=.\testorig.jpg

!IF  "$(CFG)" == "apptest - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputDir=.
OutDir=.\Release
InputPath=.\testorig.jpg

BuildCmds= \
	echo $(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testout.ppm .\testorig.jpg \
	$(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testout.ppm .\testorig.jpg \
	echo $(OutDir)\djpeg -dct int -bmp -colors 256 -outfile $(OutDir)\testout.bmp .\testorig.jpg \
	$(OutDir)\djpeg -dct int -bmp -colors 256 -outfile $(OutDir)\testout.bmp .\testorig.jpg \
	

"$(OutDir)\testout.ppm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testout.bmp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "apptest - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Testing - $(InputPath)
InputDir=.
OutDir=.\Debug
InputPath=.\testorig.jpg

BuildCmds= \
	echo $(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testout.ppm .\testorig.jpg \
	$(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testout.ppm .\testorig.jpg \
	echo $(OutDir)\djpeg -dct int -bmp -colors 256 -outfile $(OutDir)\testout.bmp .\testorig.jpg \
	$(OutDir)\djpeg -dct int -bmp -colors 256 -outfile $(OutDir)\testout.bmp .\testorig.jpg \
	

"$(OutDir)\testout.ppm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testout.bmp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\testprog.jpg

!IF  "$(CFG)" == "apptest - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputDir=.
OutDir=.\Release
InputPath=.\testprog.jpg

BuildCmds= \
	echo $(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testoutp.ppm .\testprog.jpg \
	$(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testoutp.ppm .\testprog.jpg \
	echo $(OutDir)\jpegtran -outfile $(OutDir)\testoutt.jpg .\testprog.jpg \
	$(OutDir)\jpegtran -outfile $(OutDir)\testoutt.jpg .\testprog.jpg \
	

"$(OutDir)\testoutp.ppm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testoutt.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "apptest - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Testing - $(InputPath)
InputDir=.
OutDir=.\Debug
InputPath=.\testprog.jpg

BuildCmds= \
	echo $(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testoutp.ppm .\testprog.jpg \
	$(OutDir)\djpeg -dct int -ppm -outfile $(OutDir)\testoutp.ppm .\testprog.jpg \
	echo $(OutDir)\jpegtran -outfile $(OutDir)\testoutt.jpg .\testprog.jpg \
	$(OutDir)\jpegtran -outfile $(OutDir)\testoutt.jpg .\testprog.jpg \
	

"$(OutDir)\testoutp.ppm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(OutDir)\testoutt.jpg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
