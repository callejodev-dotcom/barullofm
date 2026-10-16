; Instalador de Barullo FM para Windows (Inno Setup 6)
; Se compila desde el workflow con:  iscc /DAppVersion=0.4.0 /DArtDir=<artefactos Release> /DOutDir=<salida> packaging\windows.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef ArtDir
  #define ArtDir "..\build\BarulloFM_artefacts\Release"
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif

[Setup]
AppId={{6F0B7C52-8E1A-4C3D-9B6E-1A2B3C4D5E7F}
AppName=Barullo FM
AppVersion={#AppVersion}
AppPublisher=Escoita
AppPublisherURL=https://www.escoita.link
DefaultDirName={autopf}\Barullo FM
DefaultGroupName=Barullo FM
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=BarulloFM-{#AppVersion}-windows-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=Barullo FM

[Languages]
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Todo (VST3 e aplicación)"
Name: "custom"; Description: "Escoller"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 (para el DAW)"; Types: full custom
Name: "app";  Description: "Aplicación independente"; Types: full custom

[Files]
; VST3: va a la carpeta estándar C:\Program Files\Common Files\VST3
Source: "{#ArtDir}\VST3\Barullo FM.vst3\*"; DestDir: "{commoncf64}\VST3\Barullo FM.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\Barullo FM.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{group}\Barullo FM"; Filename: "{app}\Barullo FM.exe"; Components: app
Name: "{autodesktop}\Barullo FM"; Filename: "{app}\Barullo FM.exe"; Components: app; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Icona no escritorio"; Components: app; Flags: unchecked

[Run]
Filename: "{app}\Barullo FM.exe"; Description: "Abrir Barullo FM"; Components: app; Flags: nowait postinstall skipifsilent
