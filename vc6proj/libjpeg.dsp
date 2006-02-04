# Microsoft Developer Studio Project File - Name="libjpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libjpeg - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "libjpeg.mak" CFG="libjpeg - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "libjpeg - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "libjpeg - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libjpeg - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /YX /Zl /FD /GF /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /YX /Zl /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libjpeg - Win32 Release"
# Name "libjpeg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\jcapimin.c
# End Source File
# Begin Source File

SOURCE=.\jcapistd.c
# End Source File
# Begin Source File

SOURCE=.\jccoefct.c
# End Source File
# Begin Source File

SOURCE=.\jccolor.c
# End Source File
# Begin Source File

SOURCE=.\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jchuff.c
# End Source File
# Begin Source File

SOURCE=.\jcinit.c
# End Source File
# Begin Source File

SOURCE=.\jcmainct.c
# End Source File
# Begin Source File

SOURCE=.\jcmarker.c
# End Source File
# Begin Source File

SOURCE=.\jcmaster.c
# End Source File
# Begin Source File

SOURCE=.\jcomapi.c
# End Source File
# Begin Source File

SOURCE=.\jcparam.c
# End Source File
# Begin Source File

SOURCE=.\jcphuff.c
# End Source File
# Begin Source File

SOURCE=.\jcprepct.c
# End Source File
# Begin Source File

SOURCE=.\jcsample.c
# End Source File
# Begin Source File

SOURCE=.\jctrans.c
# End Source File
# Begin Source File

SOURCE=.\jdapimin.c
# End Source File
# Begin Source File

SOURCE=.\jdapistd.c
# End Source File
# Begin Source File

SOURCE=.\jdatadst.c
# End Source File
# Begin Source File

SOURCE=.\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=.\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=.\jdcolor.c
# End Source File
# Begin Source File

SOURCE=.\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jdhuff.c
# End Source File
# Begin Source File

SOURCE=.\jdinput.c
# End Source File
# Begin Source File

SOURCE=.\jdmainct.c
# End Source File
# Begin Source File

SOURCE=.\jdmarker.c
# End Source File
# Begin Source File

SOURCE=.\jdmaster.c
# End Source File
# Begin Source File

SOURCE=.\jdmerge.c
# End Source File
# Begin Source File

SOURCE=.\jdphuff.c
# End Source File
# Begin Source File

SOURCE=.\jdpostct.c
# End Source File
# Begin Source File

SOURCE=.\jdsample.c
# End Source File
# Begin Source File

SOURCE=.\jdtrans.c
# End Source File
# Begin Source File

SOURCE=.\jerror.c
# End Source File
# Begin Source File

SOURCE=.\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=.\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=.\jquant1.c
# End Source File
# Begin Source File

SOURCE=.\jquant2.c
# End Source File
# Begin Source File

SOURCE=.\jutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\jcolsamp.h
# End Source File
# Begin Source File

SOURCE=.\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jversion.h
# End Source File
# End Group
# Begin Group "NASM Source"

# PROP Default_Filter "asm"
# Begin Source File

SOURCE=.\jccolmmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCCOL="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jccolmmx.asm
InputName=jccolmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCCOL="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jccolmmx.asm
InputName=jccolmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jccolss2.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCCOLS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jccolss2.asm
InputName=jccolss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCCOLS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jccolss2.asm
InputName=jccolss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqnt3dn.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqnt3dn.asm
InputName=jcqnt3dn

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqnt3dn.asm
InputName=jcqnt3dn

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqntflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqntflt.asm
InputName=jcqntflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqntflt.asm
InputName=jcqntflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqntint.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqntint.asm
InputName=jcqntint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqntint.asm
InputName=jcqntint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqntmmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqntmmx.asm
InputName=jcqntmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqntmmx.asm
InputName=jcqntmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqnts2f.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqnts2f.asm
InputName=jcqnts2f

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqnts2f.asm
InputName=jcqnts2f

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqnts2i.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqnts2i.asm
InputName=jcqnts2i

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqnts2i.asm
InputName=jcqnts2i

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcqntsse.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTSS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcqntsse.asm
InputName=jcqntsse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCQNTSS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcqntsse.asm
InputName=jcqntsse

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcsammmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCSAM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcsammmx.asm
InputName=jcsammmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCSAM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcsammmx.asm
InputName=jcsammmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcsamss2.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JCSAMS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jcsamss2.asm
InputName=jcsamss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JCSAMS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jcsamss2.asm
InputName=jcsamss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdcolmmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDCOL="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdcolmmx.asm
InputName=jdcolmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDCOL="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdcolmmx.asm
InputName=jdcolmmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdcolss2.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDCOLS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdcolss2.asm
InputName=jdcolss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDCOLS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdcolss2.asm
InputName=jdcolss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmermmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDMER="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdmermmx.asm
InputName=jdmermmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDMER="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdmermmx.asm
InputName=jdmermmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmerss2.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDMERS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdmerss2.asm
InputName=jdmerss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDMERS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdmerss2.asm
InputName=jdmerss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdsammmx.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDSAM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdsammmx.asm
InputName=jdsammmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDSAM="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdsammmx.asm
InputName=jdsammmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdsamss2.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JDSAMS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jdsamss2.asm
InputName=jdsamss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JDSAMS="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jcolsamp.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jdsamss2.asm
InputName=jdsamss2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jf3dnflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JF3DN="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jf3dnflt.asm
InputName=jf3dnflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JF3DN="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jf3dnflt.asm
InputName=jf3dnflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfdctflt.asm
InputName=jfdctflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfdctflt.asm
InputName=jfdctflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctfst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfdctfst.asm
InputName=jfdctfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfdctfst.asm
InputName=jfdctfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctint.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfdctint.asm
InputName=jfdctint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFDCTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfdctint.asm
InputName=jfdctint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfmmxfst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFMMX="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfmmxfst.asm
InputName=jfmmxfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFMMX="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfmmxfst.asm
InputName=jfmmxfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfmmxint.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFMMXI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfmmxint.asm
InputName=jfmmxint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFMMXI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfmmxint.asm
InputName=jfmmxint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfss2fst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFSS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfss2fst.asm
InputName=jfss2fst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFSS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfss2fst.asm
InputName=jfss2fst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfss2int.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFSS2I="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfss2int.asm
InputName=jfss2int

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFSS2I="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfss2int.asm
InputName=jfss2int

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfsseflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JFSSE="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jfsseflt.asm
InputName=jfsseflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JFSSE="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jfsseflt.asm
InputName=jfsseflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ji3dnflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JI3DN="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\ji3dnflt.asm
InputName=ji3dnflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JI3DN="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\ji3dnflt.asm
InputName=ji3dnflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jidctflt.asm
InputName=jidctflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCT="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jidctflt.asm
InputName=jidctflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctfst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jidctfst.asm
InputName=jidctfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTF="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jidctfst.asm
InputName=jidctfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctint.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jidctint.asm
InputName=jidctint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jidctint.asm
InputName=jidctint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctred.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTR="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jidctred.asm
InputName=jidctred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIDCTR="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jidctred.asm
InputName=jidctred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jimmxfst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMX="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jimmxfst.asm
InputName=jimmxfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMX="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jimmxfst.asm
InputName=jimmxfst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jimmxint.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMXI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jimmxint.asm
InputName=jimmxint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMXI="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jimmxint.asm
InputName=jimmxint

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jimmxred.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMXR="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jimmxred.asm
InputName=jimmxred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JIMMXR="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jimmxred.asm
InputName=jimmxred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jiss2flt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jiss2flt.asm
InputName=jiss2flt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jiss2flt.asm
InputName=jiss2flt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jiss2fst.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2F="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jiss2fst.asm
InputName=jiss2fst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2F="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jiss2fst.asm
InputName=jiss2fst

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jiss2int.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2I="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jiss2int.asm
InputName=jiss2int

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2I="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jiss2int.asm
InputName=jiss2int

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jiss2red.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2R="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jiss2red.asm
InputName=jiss2red

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JISS2R="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jiss2red.asm
InputName=jiss2red

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jisseflt.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JISSE="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jisseflt.asm
InputName=jisseflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JISSE="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	"jdct.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jisseflt.asm
InputName=jisseflt

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jsimdcpu.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JSIMD="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jsimdcpu.asm
InputName=jsimdcpu

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JSIMD="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jsimdcpu.asm
InputName=jsimdcpu

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jsimdw32.asm

!IF  "$(CFG)" == "libjpeg - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__JSIMDW="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Release
InputPath=.\jsimdw32.asm
InputName=jsimdw32

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libjpeg - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__JSIMDW="$(IntDir)\jsimdcfg.inc"	"jsimdext.inc"	
# Begin Custom Build - Assembling - $(InputPath)
IntDir=.\Debug
InputPath=.\jsimdw32.asm
InputName=jsimdw32

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -Xvc -fwin32 -DWIN32 -I $(IntDir)\ -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "NASM Header"

# PROP Default_Filter "inc"
# Begin Source File

SOURCE=.\jcolsamp.inc
# End Source File
# Begin Source File

SOURCE=.\jdct.inc
# End Source File
# Begin Source File

SOURCE=.\jsimdext.inc
# End Source File
# End Group
# End Target
# End Project
