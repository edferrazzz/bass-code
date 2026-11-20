; Windows installer for Bass Code: Standalone app (Program Files) + VST3 (VST3 folder)
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef DistDir
  #define DistDir "..\dist"
#endif

[Setup]
AppId={{6E1F3B52-2C1B-4C1E-9C55-3B7A0D9B4A21}
AppName=Bass Code
AppVersion={#AppVersion}
AppPublisher=Audio Makers
DefaultDirName={autopf}\Audio Makers\Bass Code
DefaultGroupName=Bass Code
DisableProgramGroupPage=yes
DisableDirPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputBaseFilename=Bass Code {#AppVersion} Instalador
OutputDir=..\dist
SetupIconFile=..\build\BassCode_artefacts\JuceLibraryCode\icon.ico
UninstallDisplayIcon={app}\Bass Code.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "pt"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Types]
Name: "full"; Description: "App + VST3"
Name: "custom"; Description: "Escolher"; Flags: iscustom

[Components]
Name: "app"; Description: "Bass Code (app)"; Types: full custom
Name: "vst3"; Description: "Bass Code VST3 (pra usar na DAW)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; Components: app

[Files]
Source: "{#DistDir}\Bass Code.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "{#DistDir}\Bass Code.vst3\*"; DestDir: "{code:Vst3Dir}\Bass Code.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Assets\Fonts\OFL-SpaceGrotesk.txt"; DestDir: "{app}"; DestName: "Font licenses.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Bass Code"; Filename: "{app}\Bass Code.exe"; Components: app
Name: "{autodesktop}\Bass Code"; Filename: "{app}\Bass Code.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Bass Code.exe"; Description: "Abrir o Bass Code"; Components: app; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKLM; Subkey: "Software\Audio Makers\Bass Code"; ValueType: string; ValueName: "VST3Path"; ValueData: "{code:Vst3Dir}"; Flags: uninsdeletekey

[UninstallDelete]
Type: filesandordirs; Name: "{reg:HKLM\Software\Audio Makers\Bass Code,VST3Path|{commoncf64}\VST3}\Bass Code.vst3"

[Code]
var
  Vst3Page: TInputDirWizardPage;

procedure InitializeWizard;
begin
  Vst3Page := CreateInputDirPage(wpSelectComponents, 'Pasta do VST3',
    'Onde o VST3 vai ser instalado?',
    'A padr' + #227 + 'o ' + #233 + ' a pasta VST3 que todas as DAWs procuram. Clique em Procurar para escolher outra.',
    False, '');
  Vst3Page.Add('');
  Vst3Page.Values[0] := GetPreviousData('VST3Path', ExpandConstant('{commoncf64}\VST3'));
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'VST3Path', Vst3Page.Values[0]);
end;

function Vst3Dir(Param: String): String;
begin
  Result := Vst3Page.Values[0];
end;
