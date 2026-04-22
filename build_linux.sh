#!/bin/bash
# Build Zep Notification App for Linux
# Usage: ./build_linux.sh [Debug|Release]

set -e

CONFIG=${1:-Release}
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building Zep Notification App for Linux ==="

# Install dependencies (Ubuntu/Debian)
if command -v apt-get &> /dev/null; then
    echo "Installing dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        cmake \
        build-essential \
        libsdl2-dev \
        libgl1-mesa-dev \
        libfreetype6-dev \
        libx11-dev \
        libxcursor-dev \
        libxrandr \
        libxi-dev
fi

# Update submodules
echo "Updating submodules..."
git submodule update --init

# Build Zep library first
echo "Building Zep library..."
rm -rf build
mkdir -p build
cd build
cmake -G "Unix Makefiles" \
    -DBUILD_QT=OFF \
    -DBUILD_IMGUI=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_DEMOS=ON \
    -DCMAKE_BUILD_TYPE=$CONFIG \
    ..
cmake --build . --config $CONFIG

# Build notification app
echo "Building notification app..."
cd "$SCRIPT_DIR/notification_app"
rm -rf build
mkdir -p build
cd build
cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=$CONFIG \
    ..
cmake --build . --config $CONFIG

# Install
echo "Installing..."
cmake --install . --config $CONFIG --prefix ../install/linux

echo "=== Build complete ==="
echo "Binary: install/linux/bin/ZepNotificationApp"