; =============================================================================
; Atlas Browser Windows Installer (NSIS)
; =============================================================================
;
; Build with: makensis atlas_installer.nsi
;

!include "MUI2.nsh"
!include "FileFunc.nsh"

; -----------------------------------------------------------------------------
; General Settings
; -----------------------------------------------------------------------------

Name "Atlas Browser"
OutFile "AtlasBrowserSetup.exe"
InstallDir "$PROGRAMFILES64\Atlas Browser"
InstallDirRegKey HKLM "Software\AtlasBrowser" "InstallDir"
RequestExecutionLevel admin

; Version info
!define PRODUCT_NAME "Atlas Browser"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "Atlas Browser Authors"
!define PRODUCT_WEB_SITE "https://atlasbrowser.com"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\AtlasBrowser"

VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "FileDescription" "Atlas Browser Installer"
VIAddVersionKey "LegalCopyright" "Copyright 2025 Atlas Browser Authors"

; -----------------------------------------------------------------------------
; Modern UI Settings
; -----------------------------------------------------------------------------

!define MUI_ABORTWARNING
!define MUI_ICON "..\..\src\atlas_browser\resources\icons\atlas.ico"
!define MUI_UNICON "..\..\src\atlas_browser\resources\icons\atlas.ico"

; Welcome page
!define MUI_WELCOMEPAGE_TITLE "Welcome to Atlas Browser Setup"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of Atlas Browser.$\r$\n$\r$\nAtlas Browser is an AI-powered browser with built-in agent capabilities.$\r$\n$\r$\nClick Next to continue."

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\atlas.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Atlas Browser"

; -----------------------------------------------------------------------------
; Pages
; -----------------------------------------------------------------------------

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; -----------------------------------------------------------------------------
; Languages
; -----------------------------------------------------------------------------

!insertmacro MUI_LANGUAGE "English"

; -----------------------------------------------------------------------------
; Installer Sections
; -----------------------------------------------------------------------------

Section "Atlas Browser" SecMain
    SectionIn RO
    
    SetOutPath "$INSTDIR"
    
    ; Copy main executable and dependencies
    File "atlas.exe"
    File "*.dll"
    File "*.pak"
    File "*.dat"
    File "*.bin"
    
    ; Copy subdirectories
    SetOutPath "$INSTDIR\locales"
    File /r "locales\*.*"
    
    SetOutPath "$INSTDIR\resources"
    File /r "resources\*.*"
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\Atlas Browser"
    CreateShortcut "$SMPROGRAMS\Atlas Browser\Atlas Browser.lnk" "$INSTDIR\atlas.exe"
    CreateShortcut "$SMPROGRAMS\Atlas Browser\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortcut "$DESKTOP\Atlas Browser.lnk" "$INSTDIR\atlas.exe"
    
    ; Write registry keys
    WriteRegStr HKLM "Software\AtlasBrowser" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\AtlasBrowser" "Version" "${PRODUCT_VERSION}"
    
    ; Write uninstaller registry keys
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\atlas.exe"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    
    ; Calculate installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
    
    ; Register as browser
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser" "" "Atlas Browser"
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\DefaultIcon" "" "$INSTDIR\atlas.exe,0"
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\shell\open\command" "" '"$INSTDIR\atlas.exe"'
    
    ; URL associations
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\Capabilities" "ApplicationDescription" "AI-powered browser"
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\Capabilities" "ApplicationName" "Atlas Browser"
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\Capabilities\URLAssociations" "http" "AtlasBrowserURL"
    WriteRegStr HKLM "Software\Clients\StartMenuInternet\AtlasBrowser\Capabilities\URLAssociations" "https" "AtlasBrowserURL"
    
    WriteRegStr HKLM "Software\Classes\AtlasBrowserURL" "" "Atlas Browser URL"
    WriteRegStr HKLM "Software\Classes\AtlasBrowserURL\shell\open\command" "" '"$INSTDIR\atlas.exe" "%1"'
    
    WriteRegStr HKLM "Software\RegisteredApplications" "Atlas Browser" "Software\Clients\StartMenuInternet\AtlasBrowser\Capabilities"
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; -----------------------------------------------------------------------------
; Uninstaller Section
; -----------------------------------------------------------------------------

Section "Uninstall"
    ; Remove shortcuts
    Delete "$DESKTOP\Atlas Browser.lnk"
    RMDir /r "$SMPROGRAMS\Atlas Browser"
    
    ; Remove installation directory
    RMDir /r "$INSTDIR"
    
    ; Remove registry keys
    DeleteRegKey HKLM "Software\AtlasBrowser"
    DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "Software\Clients\StartMenuInternet\AtlasBrowser"
    DeleteRegKey HKLM "Software\Classes\AtlasBrowserURL"
    DeleteRegValue HKLM "Software\RegisteredApplications" "Atlas Browser"
SectionEnd
