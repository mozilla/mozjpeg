!include x64.nsh
Name "libjpeg-turbo SDK for ${PLATFORM}"
OutFile ${WBLDDIR}\${APPNAME}.exe
InstallDir c:\${APPNAME}

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "libjpeg-turbo SDK for ${PLATFORM} (required)"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif
	SectionIn RO
!ifdef GCC
	IfFileExists $SYSDIR/libturbojpeg.dll exists 0
!else
	IfFileExists $SYSDIR/turbojpeg.dll exists 0
!endif
	goto notexists
	exists:
!ifdef GCC
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for ${PLATFORM} is already installed.  Please uninstall it first."
!else
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for ${PLATFORM} or the TurboJPEG SDK is already installed.  Please uninstall it first."
!endif
	quit

	notexists:
	SetOutPath $SYSDIR
!ifdef GCC
	File "${WLIBDIR}\libturbojpeg.dll"
!else
	File "${WLIBDIR}\turbojpeg.dll"
!endif
	SetOutPath $INSTDIR\bin
!ifdef GCC
	File "/oname=libjpeg-62.dll" "${WLIBDIR}\libjpeg-*.dll" 
!else
	File "${WLIBDIR}\jpeg62.dll"
!endif
	SetOutPath $INSTDIR\lib
!ifdef GCC
	File "${WLIBDIR}\libturbojpeg.dll.a"
	File "${WLIBDIR}\libturbojpeg.a"
	File "${WLIBDIR}\libjpeg.dll.a"
	File "${WLIBDIR}\libjpeg.a"
!else
	File "${WLIBDIR}\turbojpeg.lib"
	File "${WLIBDIR}\turbojpeg-static.lib"
	File "${WLIBDIR}\jpeg.lib"
	File "${WLIBDIR}\jpeg-static.lib"
!endif
	SetOutPath $INSTDIR\include
	File "${WHDRDIR}\jconfig.h"
	File "${WSRCDIR}\jerror.h"
	File "${WSRCDIR}\jmorecfg.h"
	File "${WSRCDIR}\jpeglib.h"
	File "${WSRCDIR}\turbojpeg.h"
	SetOutPath $INSTDIR
	File "${WSRCDIR}\README"
	File "${WSRCDIR}\README-turbo.txt"
	File "${WSRCDIR}\libjpeg.doc"
	File "${WSRCDIR}\LGPL.txt"
	File "${WSRCDIR}\LICENSE.txt"

	WriteRegStr HKLM "SOFTWARE\${APPNAME} ${VERSION}" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} ${VERSION}" "DisplayName" "libjpeg-turbo SDK v${VERSION} for ${PLATFORM}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} ${VERSION}" "UninstallString" '"$INSTDIR\uninstall_${VERSION}.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} ${VERSION}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} ${VERSION}" "NoRepair" 1
	WriteUninstaller "uninstall_${VERSION}.exe"
SectionEnd

Section "Uninstall"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif

	SetShellVarContext all

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME} ${VERSION}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME} ${VERSION}"

!ifdef GCC
	Delete $INSTDIR\bin\libjpeg-62.dll
	Delete $SYSDIR\libturbojpeg.dll
	Delete $INSTDIR\lib\libturbojpeg.dll.a"
	Delete $INSTDIR\lib\libturbojpeg.a"
	Delete $INSTDIR\lib\libjpeg.dll.a"
	Delete $INSTDIR\lib\libjpeg.a"
!else
	Delete $INSTDIR\bin\jpeg62.dll
	Delete $SYSDIR\turbojpeg.dll
	Delete $INSTDIR\lib\jpeg.lib
	Delete $INSTDIR\lib\jpeg-static.lib
	Delete $INSTDIR\lib\turbojpeg.lib
	Delete $INSTDIR\lib\turbojpeg-static.lib
!endif
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
	RMDir "$INSTDIR\bin"
	RMDir "$INSTDIR"

SectionEnd
