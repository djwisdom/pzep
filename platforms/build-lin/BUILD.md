# pZep Build Configuration - Linux

## Linux Build Script

### Requirements
- GCC 9+ or Clang 10+
- CMake 3.15+

### Build

```bash
# TUI only (no dependencies)
mkdir -p build-lin
cd build-lin
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Or with GUI (requires SDL2, ImGui, GLAD, freetype)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_IMGUI=ON
cmake --build .
```

### Dependencies (GUI)

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev libgl1-mesa-dev libfreetype6-dev

# Or via package manager of choice
```

### Running

```bash
# TUI version
./pzep-tui file.txt

# GUI version  
./pzep-gui file.txt
```