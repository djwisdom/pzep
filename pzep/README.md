# pZep
## A Standalone Vim-Like Editor

A lightweight, embeddable Vim-like text editor with full Vim emulation powered by Raylib.

---

## Structure

```
pzep/
  pzep-tui/      # Terminal UI version (coming soon)
  pzep-gui/      # GUI (Raylib) version
  platforms/    # Build configurations
    build-win/  # Windows build
    build-lin/  # Linux build
    build-bsd/  # FreeBSD build
```

---

## Quick Start

### Build for your platform

**Windows:**
```powershell
# Install Raylib via vcpkg
vcpkg install raylib:x64-windows

# Build
cd platforms/build-win
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**Linux:**
```bash
# Install Raylib
sudo apt install libraylib-dev

# Build
cd platforms/build-lin
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**FreeBSD:**
```bash
# Install Raylib
sudo pkg install raylib

# Build
cd platforms/build-bsd
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

---

## Versions

### pzep-tui
Terminal-based rendering (coming soon).

### pzep-gui  
GUI rendering using **Raylib** - hardware accelerated, cross-platform.

---

## Features

- Full Vim emulation (~75% compatible)
- Ex commands (~30% implemented)
- Multiple file editing (tabs/splits)
- Syntax highlighting
- Git integration
- Macros, folds, visual mode
- Multiple cursors
- Minimap
- Hardware-accelerated rendering (OpenGL)

---

## Documentation

- [User Guide](../docs/USER_GUIDE.md)
- [Vim Compatibility Report](../docs/VIM_COMPATIBILITY_REPORT.md)
- [Vim Commands Report](../docs/VIM_COMMANDS_REPORT.md)
- [Technical Debt Report](../docs/TECHNICAL_DEBT_REPORT.md)