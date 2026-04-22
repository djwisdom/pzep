# pzep-gui
## A GUI-Based Vim-Like Editor

A standalone GUI implementation of the pZep editor using **Raylib** for rendering.

---

## Features

- Full Vim emulation (motion, operators, ex commands)
- Raylib renderer - hardware accelerated with OpenGL
- Multiple file editing with tabs/splits
- Syntax highlighting
- Git integration
- Macros and folds
- Multiple cursors
- Minimap
- Cross-platform (Windows, Linux, FreeBSD)

---

## Building

### Prerequisites

**Windows:**
```powershell
vcpkg install raylib:x64-windows
```

**Linux:**
```bash
sudo apt install libraylib-dev
```

**FreeBSD:**
```bash
sudo pkg install raylib
```

### Build

```bash
# Create build directory
mkdir build
cd build

# Configure (using vcpkg on Windows)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .
```

### Running

```bash
./pzep-gui file.txt
```

Or just:

```bash
./pzep-gui
```

For an empty buffer.

---

## Keybindings

See the [User Guide](../docs/USER_GUIDE.md) for the full reference.

Quick start:
- `i` - Insert mode
- `ESC` - Normal mode
- `:w` - Save
- `:q` - Quit
- `h/j/k/l` - Navigate