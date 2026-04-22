# pZep - Vim-like Code Editor

A lightweight, embeddable code editor with full Vim keybindings.

## Architecture

### Display Backends

pZep supports multiple display backends through the `ZepDisplay` interface:

- **Raylib** (`src/raylib/`) - Graphics window (OpenGL/Desktop)
- **FTXUI** (`src/ftxui/`) - Terminal UI (experimental)
- **ImGui** (`src/imgui/`) - Dear ImGui integration

### Editor Structure (Four-Corner Layout)

The Zep editor uses a **region-based layout system**:

```
┌─────────────────────────────────────────────┐
│ Tab Region (top)                           │ ← File tabs (multiple files)
├─────────────────────────────────────────┤
│ Tab Content Region (center)                 │
│ ┌───────┬──────────┬──────┬───────────┐  │
│ │ Line  │ Indicator│ Text │ Scrollbar │  │
│ │ nums │ (git)   │      │ /minimap │  │
│ └───────┴──────────┴──────┴───────────┘  │
├──────────────────────────────────────────┤
│ Command Region (bottom)                   │
│ Vim | NORMAL | 100% | untitled | 0:0  │ ← Status bar
└─────────────────────────────────────────────┘
```

**Key Components:**

- `ZepDisplay` - Display interface (rendering, input)
- `ZepEditor` - Main editor controller
- `ZepTabWindow` - Tab container (multiple files/panes)
- `ZepWindow` - Editor window (text area, line numbers, scrollbar)
- `ZepMode` - Input mode (Normal, Insert, Visual)

### Vim Modes

| Mode | Description |
|------|-----------|
| Normal | Default Navigation |
| Insert | Text Editing |
| Visual | Text Selection |
| Command | Ex Commands (`:w`, `:q`) |

### Building

```bash
# Build Zep library
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release

# Build pZep-GUI
cd apps/pzep-gui
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Running

```
./pzep_gui [filename]

# Keybindings:
i - Enter insert mode
ESC - Return to normal mode
:q - Quit
:w - Save
```

## Repository Structure

```
pzep/
├── apps/
│   └── pzep-gui/     # pZep-GUI application (Raylib)
├── src/               # Zep library source
│   ├── ftxui/         # FTXUI display
│   ├── imgui/         # ImGui display
│   └── raylib/        # Raylib display
├── include/           # Zeppelin headers
├── tests/            # Unit tests
├── build/            # Build output
└── README.md
```

## Key Files

| File | Purpose |
|------|--------|
| `src/editor.cpp` | Main editor logic, layout |
| `src/window.cpp` | Window rendering (4-corner layout) |
| `src/mode_vim.cpp` | Vim mode implementation |
| `src/terminal.cpp` | Terminal emulator |
| `src/buffer.cpp` | Text buffer |

## Distribution

### Installation Scopes

#### 1. Per-User (Corporate/PIM policy)
Installs to user directory, no admin rights required:
- **Location**: `%LOCALAPPDATA%\pZep` (Windows)
- **Silent install**: `pZep-setup.exe /CURRENTUSER`
- **Uninstall**: `%LOCALAPPDATA%\pZep\uninstall.exe`

#### 2. All-Users (End consumers)
Installs system-wide, requires admin:
- **Location**: `%ProgramFiles%\pZep` (Windows)
- **Silent install**: `pZep-setup.exe /ALLUSERS`
- **Uninstall**: `%ProgramFiles%\pZep\uninstall.exe`

### Dependencies (bundled automatically on build)

- raylib.dll
- glfw3.dll
- freetype.dll
- libpng16.dll
- zlib1.dll

### Packaging Options

**1. Inno Setup (recommended)**
```iss
; pZep-GUI.iss
[Setup]
AppName=pZep
DefaultDirName={pf}\pZep
DefaultGroupName=pZep
OutputDir=dist
OutputBaseFilename=pZep-setup
AllowUserView=Yes
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

[Registry]
; Per-user install (default, supports PIM)
Root: HKCU; Subkey: "Software\pZep"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey
; All-users install
Root: HKLM; Subkey: "Software\pZep"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssUserInfo then
    WizardForm.NoIconsCheck.Visible := True;
end;
```

**2. MSIX/AppX (Microsoft Store ready)**
```xml
<!-- AppxManifest.xml -->
<Package>
  <Identity Name="pZep" Version="0.1.0"/>
  <Resources>
    <Resource Target="User"/>
  </Resources>
</Package>
```

**3. winget:**
```yaml
# winget-pkgs/pZep/pZep.yaml
PackageName: pZep
Version: 0.1.0
Publisher: djwisdom
Installers:
  - Architecture: x64
    InstallerUrl: https://github.com/djwisdom/pzep/releases/latest/download/pZep-setup.exe
    InstallerScope: user
  - Architecture: x64
    InstallerUrl: https://github.com/djwisdom/pzep/releases/latest/download/pZep-setup.exe
    InstallerScope: machine
```

**4. Chocolatey:**
```sh
# Per-user
choco install pzep -y --scope=user

# All-users  
choco install pzep -y --scope=machine
```

## Credits

Fork of Zep. Originally based on nzep (Notification Editor).