  !include sections.nsh

;General

  ;Name and file
  Name "TibiaMovie"
  OutFile "TibiaMovieSetup.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\TibiaMovie"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\TibiaMovie" ""

;--------------------------------
;Interface Settings

  ShowInstDetails hide
  XPStyle on

;--------------------------------
;Pages

  PageEx license
    LicenseData "COPYING"
  PageExEnd
  Page components
  Page directory
  Page instfiles

  Uninstpage uninstConfirm
  Uninstpage instfiles

;Installer Sections

Section "TibiaMovie" SecDummy
  ; Can't disable this section
  SectionIn RO

  SetOutPath "$INSTDIR"
  
  ;List of files
  File TibiaMovie.exe
  File zlib1.dll
  File readme.txt

  ;Store installation folder
  WriteRegStr HKCU "Software\TibiaMovie" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TibiaMovie" "DisplayName" "TibiaMovie"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TibiaMovie" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TibiaMovie" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TibiaMovie" "NoRepair" 1

SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\TibiaMovie"
  CreateShortCut "$SMPROGRAMS\TibiaMovie\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\TibiaMovie\TibiaMovie.lnk" "$INSTDIR\TibiaMovie.exe" "" "$INSTDIR\TibiaMovie.exe" 0
  
SectionEnd

Section "Desktop Shortcut"
  CreateShortCut "$DESKTOP\TibiaMovie.lnk" "$INSTDIR\TibiaMovie.exe"
SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
  Delete "$INSTDIR\TibiaMovie.exe"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\TibiaMovie\TibiaMovie.lnk"
  Delete "$SMPROGRAMS\TibiaMovie\Uninstall.lnk"
  RMDir "$SMPROGRAMS\TibiaMovie"
  Delete "$DESKTOP\TibiaMovie.lnk"

  DeleteRegKey /ifempty HKCU "Software\TibiaMovie"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TibiaMovie"

SectionEnd