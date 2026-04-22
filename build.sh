#!/bin/bash
# Cross-platform build for Zep Notification App
# Usage: ./build.sh [linux|freebsd|macos|windows] [Debug|Release]

set -e

PLATFORM=${1:-$(detect_platform)}
CONFIG=${2:-Release}

detect_platform() {
    case "$OSTYPE" in
        linux*)    echo "linux" ;;
        darwin*)   echo "macos" ;;
        freebsd*)  echo "freebsd" ;;
        win32*)    echo "windows" ;;
        msys*)    echo "windows" ;;
        *)        echo "linux" ;;
    esac
}

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building Zep Notification App for $PLATFORM ($CONFIG) ==="

# Update submodules
echo "Updating submodules..."
git submodule update --init 2>/dev/null || true

# Platform-specific build
case "$PLATFORM" in
    linux)
        echo "Building for Linux..."
        chmod +x ./build_linux.sh
        ./build_linux.sh "$CONFIG"
        ;;
    freebsd)
        echo "Building for FreeBSD..."
        chmod +x ./build_freebsd.sh
        ./build_freebsd.sh "$CONFIG"
        ;;
    macos)
        echo "Building for macOS..."
        # Use config_imgui.sh and build.sh like existing demos
        rm -rf build
        mkdir -p build
        cd build
        cmake -G "Unix Makefiles" \
            -DBUILD_QT=OFF \
            -DBUILD_IMGUI=ON \
            -DBUILD_TESTS=OFF \
            -DBUILD_DEMOS=ON \
            -DCMAKE_BUILD_TYPE="$CONFIG" \
            ..
        cmake --build . --config "$CONFIG"
        # Build notification app
        cd "$SCRIPT_DIR/notification_app"
        rm -rf build
        mkdir -p build
        cd build
        cmake -G "Unix Makefiles" \
            -DCMAKE_BUILD_TYPE="$CONFIG" \
            ..
        cmake --build . --config "$CONFIG"
        ;;
    windows)
        echo "Building for Windows..."
        cmd /c "config_imgui.bat $CONFIG"
        cmake --build . --config "$CONFIG"
        ;;
esac

echo "=== Build complete ==="
echo "Platform: $PLATFORM"
echo "Config: $CONFIG"
echo "Output: install/$PLATFORM/bin/"