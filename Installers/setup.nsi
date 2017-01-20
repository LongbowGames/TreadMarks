!addPluginDir "."

; Include modern UI
!include "MUI.nsh"
!include "WinVer.nsh"

!define BANNER "TM_Install.bmp"

; ParallaxTZ's RMDirUp starts
Function un.RMDirUP
         !define RMDirUP '!insertmacro RMDirUPCall'
 
         !macro RMDirUPCall _PATH
                push '${_PATH}'
                Call un.RMDirUP
         !macroend
 
         ; $0 - current folder
         ClearErrors
 
         Exch $0
         ;DetailPrint "ASDF - $0\.."
         RMDir "$0\.."
         
         IfErrors Skip
         ${RMDirUP} "$0\.."
         Skip:
         
         Pop $0
FunctionEnd
; ParallaxTZ's RMDirUp ends

; REGKEY is used under both HKLM and NKCU (the latter for the IGF install only)
!define PRODUCT "Tread Marks"
!define EXENAME "TreadMarks"
!define GAMEEXENAME "TM"
!define REGKEY "Software\Longbow Digital Arts\${PRODUCT}"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"
!define WEBSITE "http://www.LongbowGames.com/"
!define MYAPPDATADIR "$APPDATA\Longbow Digital Arts\${PRODUCT}"

; Name of the installer, output file, and default directory/registry key
Name "Tread Marks"
OutFile "out/Tread Marks Install.exe"
InstallDir "$PROGRAMFILES\LDA Games\${PRODUCT}"
InstallDirRegKey HKLM "${REGKEY}" "Install_Dir"
SetCompressor /SOLID lzma
RequestExecutionLevel admin

; variables
Var START_FOLDER
Var MUI_TEMP
Var EXPANSION

; misc
!define MUI_ICON "install.ico"
!define MUI_UNICON "uninstall.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_UNHEADERIMAGE_BITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP ${BANNER}
!define MUI_UNWELCOMEFINISHPAGE_BITMAP ${BANNER}
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR/${EXENAME}.exe"
!define MUI_FINISHPAGE_RUN_TEXT "${PRODUCT}"

; pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\out\LICENSE"
!define MUI_PAGE_CUSTOMFUNCTION_PRE expansionSkip
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_PRE expansionSkip
!insertmacro MUI_PAGE_DIRECTORY

; This is the name of the start menu folder we'll use.
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${PRODUCT}"
; This is the key that stores what our start menu folder is, so that we can delete it on uninstall.
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${REGKEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

!define MUI_PAGE_CUSTOMFUNCTION_PRE expansionSkip
!insertmacro MUI_PAGE_STARTMENU StartMenu $START_FOLDER

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; instfiles page *****************
; The stuff to install
Section "${PRODUCT} (Required)"
  SetShellVarContext all
  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath "$INSTDIR"

  ; All the files we want
  File /r /x *.svn /x *.db ..\out\*

  ; Write the installation path into the registry
  WriteRegStr HKLM "${REGKEY}" "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "${UNINSTKEY}" "DisplayName" "${PRODUCT}"
  WriteRegStr HKLM "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall-v2.exe"'
  WriteUninstaller "uninstall-v2.exe"

  ; Start menu directory
  ${If} $EXPANSION == "0"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN StartMenu
      CreateDirectory "$SMPROGRAMS\$START_FOLDER"
      CreateShortCut "$SMPROGRAMS\$START_FOLDER\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}.exe"
      CreateShortCut "$SMPROGRAMS\$START_FOLDER\Longbow Games Web Site.lnk" "${WEBSITE}"
    !insertmacro MUI_STARTMENU_WRITE_END
  ${EndIf}
SectionEnd

Section "Desktop Shortcut" deskicon_section
  SetShellVarContext all
  CreateShortcut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${EXENAME}.exe"
SectionEnd

; uninstall stuff

; special uninstall section.
Section "Uninstall"
  SetShellVarContext all
  !insertmacro MUI_STARTMENU_GETFOLDER StartMenu $MUI_TEMP

  ; remove registry keys
  DeleteRegKey HKLM "${UNINSTKEY}"
  DeleteRegKey HKLM "${REGKEY}"

  ; remove files
  RMDir /r "$INSTDIR"
  ${RMDirUp} "$INSTDIR"
  RMDir /r "$SMPROGRAMS\$MUI_TEMP"
  ${RMDirUp} "$SMPROGRAMS\$MUI_TEMP"
  Delete "$DESKTOP\${PRODUCT}.lnk"
SectionEnd

Function expansionSkip
  ; We want to skip this page because we're installing an expansion
  ${If} $EXPANSION == "1"
    Abort
  ${EndIf}
FunctionEnd

Function .onInit
  StrCpy $EXPANSION "0"

  ; Make sure they're an administrator
  ClearErrors
  UserInfo::GetName
  ${Unless} ${Errors}
    Pop $0
    UserInfo::GetAccountType
    Pop $1

    ${If} $1 != "Admin"
      MessageBox MB_OK|MB_ICONEXCLAMATION "You must be an administrator to install ${PRODUCT}."
      Abort
    ${EndIf}
  ${EndUnless}

  ; default for Vista is no desktop icon
  ; Also, don't install icon if we're installing an expansion pack
  ${If} ${AtLeastWinVista}
  ${OrIf} $EXPANSION == "1"
    SectionSetFlags ${deskicon_section} 0
  ${EndIf}
FunctionEnd

