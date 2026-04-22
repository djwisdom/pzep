# pZep Build Configuration - Windows

## Windows Build Script

### Requirements
- Visual Studio 2019+ or Build Tools for VS 2022
- CMake 3.15+

### Build

```powershell
# Create build directory
mkdir build-win
cd build-win

# Configure (TUI only - no dependencies)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Or with GUI (requires ImGui, GLAD, SDL2)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_IMGUI=ON

# Build
cmake --build . --config Release

# Run
.\Release\pzep-tui.exe file.txt
```

### Dependencies (Optional - for GUI)

Install via vcpkg:
```powershell
vcpkg install sdl2 imgui glad freetype
```

Then configure with:
```cmake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_IMGUI=ON -DCMAKE_TOOLCHAIN_FILE=C:\\vcpkg\\scripts\\buildsystems\\vcpkg.cmake
```