# pzep-tui: Terminal-Based Vim-Like Editor using FTXUI

## Build Instructions

### Prerequisites

- C++20 compiler
- CMake 3.15+
- FTXUI library

### Install Dependencies

**Windows (vcpkg):**
```powershell
vcpkg install ftxui:x64-windows
```

**Linux:**
```bash
# Debian/Ubuntu
sudo apt install libftxui-dev

# Fedora/RHEL
sudo dnf install ftxui

# Arch
sudo pacman -S ftxui
```

**macOS:**
```bash
brew install ftxui
```

**FreeBSD:**
```bash
sudo pkg install ftxui
```

### Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .
```

### Running

```bash
./pzep-tui [filename]
```

## Keybindings

| Key | Action |
|-----|--------|
| `h/j/k/l` | Navigation |
| `i` | Insert mode |
| `ESC` | Normal mode |
| `:` | Ex command mode |
| `w/b` | Word motion |
| `0/$` | Line motion |
| `gg/G` | File motion |

## Ex Commands

- `:w` - Save file
- `:q` - Quit
- `:q!` - Quit without saving
- `:e <file>` - Open file
- `:bn` - Next buffer
- `:bp` - Previous buffer
- `:sp <file>` - Split horizontally
- `:set <option>` - Toggle options

## Architecture

pZep-tui uses FTXUI for terminal rendering:

- **Display Backend**: `ZepDisplay_FTXUI` implements `ZepDisplay` interface
- **Font Handling**: `ZepFont_FTXUI` manages terminal font metrics
- **Input**: FTXUI event loop handles keyboard/mouse input
- **Rendering**: Element-based declarative UI (React-style)

## Note on Terminal Support

FTXUI requires a terminal emulator (not raw TTY). It works with:
- Terminal emulators (iTerm2, Windows Terminal, kitty, etc.)
- SSH sessions
- screen/tmux

For true TTY support, consider the NCurses backend (planned future enhancement).