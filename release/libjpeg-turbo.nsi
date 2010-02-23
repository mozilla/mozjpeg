Name "libjpeg-turbo SDK"
OutFile ${WSRCDIR}\libjpeg-turbo.exe
InstallDir ${WINSTDIR}

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "libjpeg-turbo SDK (required)"
	SectionIn RO
	SetOutPath $SYSDIR
	File "${WSRCDIR}\turbojpeg.dll"
	File "${WSRCDIR}\jpeg62.dll"
	SetOutPath $INSTDIR\lib
	File "${WSRCDIR}\turbojpeg.lib"
	File "${WSRCDIR}\turbojpeg-static.lib"
	File "${WSRCDIR}\jpeg.lib"
	File "${WSRCDIR}\jpeg-static.lib"
	SetOutPath $INSTDIR\include
	File "win\jconfig.h"
	File "jerror.h"
	File "jmorecfg.h"
	File "jpeglib.h"
	File "turbojpeg.h"
	SetOutPath $INSTDIR
	File "README"
	File "README-turbo.txt"
	File "libjpeg.doc"
	File "LGPL.txt"
	File "LICENSE.txt"

	WriteRegStr HKLM "SOFTWARE\libjpeg-turbo ${VERSION}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo ${VERSION}" "DisplayName" "libjpeg-turbo SDK ${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo ${VERSION}" "UninstallString" '"$INSTDIR\uninstall_${VERSION}.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo ${VERSION}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo ${VERSION}" "NoRepair" 1
	WriteUninstaller "uninstall_${VERSION}.exe"
SectionEnd

Section "Uninstall"

	SetShellVarContext all

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo ${VERSION}"
	DeleteRegKey HKLM "SOFTWARE\libjpeg-turbo ${VERSION}"

	Delete $SYSDIR\jpeg62.dll
	Delete $SYSDIR\turbojpeg.dll
	Delete $INSTDIR\lib\jpeg.lib
	Delete $INSTDIR\lib\jpeg-static.lib
	Delete $INSTDIR\lib\turbojpeg.lib
	Delete $INSTDIR\lib\turbojpeg-static.lib
	Delete $INSTDIR\include\jconfig.h"
	Delete $INSTDIR\include\jerror.h"
	Delete $INSTDIR\include\jmorecfg.h"
	Delete $INSTDIR\include\jpeglib.h"
	Delete $INSTDIR\include\turbojpeg.h"
	Delete $INSTDIR\uninstall_${VERSION}.exe
	Delete $INSTDIR\README
	Delete $INSTDIR\README-turbo.txt
	Delete $INSTDIR\libjpeg.doc
	Delete $INSTDIR\LGPL.txt
	Delete $INSTDIR\LICENSE.txt

	RMDir "$INSTDIR\include"
	RMDir "$INSTDIR\lib"
	RMDir "$INSTDIR"

SectionEnd
