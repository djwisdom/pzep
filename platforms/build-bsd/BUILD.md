# pZep Build Configuration - FreeBSD

## FreeBSD Build Script

### Requirements
- GCC 11+ or Clang 13+
- CMake 3.15+

### Build

```bash
# TUI only (no dependencies)
mkdir -p build-bsd
cd build-bsd
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Or with GUI (requires SDL2, ImGui, GLAD, freetype)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_IMGUI=ON
cmake --build .
```

### Dependencies (GUI)

```bash
# FreeBSD
sudo pkg install sdl2 glew freetype2

# Or compile from ports
```

### Running

```bash
# TUI version
./pzep-tui file.txt

# GUI version  
./pzep-gui file.txt
```

### Notes

- FreeBSD uses libc++ by default, but pZep requires libstdc++ for some features
- Pass `-DCMAKE_CXX_FLAGS="-stdlib=libstdc++"` if needed