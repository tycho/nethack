; Sporkhack installer for Windows binaries
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It's pretty much flagrantly nicked from the 'example' NSIS scripts,
; since I'm learning NSIS only to install this.  :)
;--------------------------------

; The name of the installer
Name "SporkHack"

; The file to write
OutFile "sporkhack_setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\SporkHack

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\SporkHack" "Install_Dir"

;--------------------------------

; Pages

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "SporkHack game files (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put files there
  File d:\nh\distro\*.*

  ; create blank 'dumps' directory
  CreateDirectory "$INSTDIR\dumps"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\SporkHack "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SporkHack" "DisplayName" "SporkHack"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SporkHack" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SporkHack" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SporkHack" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\SporkHack"
  CreateShortCut "$SMPROGRAMS\SporkHack\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\SporkHack\SporkHack.lnk" "$INSTDIR\Nethack.exe" "" "$INSTDIR\Nethack.exe" 0
  
SectionEnd

;--------------------------------
; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SporkHack"
  DeleteRegKey HKLM SOFTWARE\SporkHack

  ; Remove files and uninstaller
  Delete $INSTDIR\*.*
  Delete $INSTDIR\dumps\*.*
  RMDir "$INSTDIR\dumps"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\SporkHack\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\SporkHack"
  RMDir "$INSTDIR"

SectionEnd
