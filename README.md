# pZep - A Vim-based Code Editor

A lightweight, embeddable code editor with Vim keybindings. Fork of Zep (from nzep).

## Features

- Vim keybindings (Normal/Insert/Visual modes)
- Syntax highlighting
- Multiple display backends (Raylib, ImGui, Qt)
- Tabs and splits

## Building pZep-GUI

The pZep-GUI application uses Raylib for rendering:

```bash
cd apps/pzep-gui
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Usage

- `i` - Enter insert mode
- `ESC` - Return to normal mode
- `:q` - Quit
- `:wq` - Save and quit

## Repository Structure

```
pzep/
├── apps/
│   └── pzep-gui/     # pZep-GUI application (Raylib)
├── src/               # Zep library source
├── include/           # Zep headers
├── tests/            # Unit tests
└── README.md
```

## Credit

Based on [Zep](https://github.com/zepeditor/zep) by f闭幕马拉