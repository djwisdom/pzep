#!/bin/sh
# Build Zep Notification App for FreeBSD
# Usage: ./build_freebsd.sh [Debug|Release]

set -e

CONFIG=${1:-Release}
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Building Zep Notification App for FreeBSD ==="

# Install dependencies (FreeBSD pkg)
echo "Installing dependencies..."
if command -v pkg &> /dev/null; then
    pkg install -y \
        cmake \
        devel/gmake \
        graphics/sdl2 \
        graphics/mesa-libs \
        freetype \
        x11/libX11 \
        x11/libXcursor \
        x11/libXrandr \
        x11/libXi
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
cmake --install . --config $CONFIG --prefix ../install/freebsd

echo "=== Build complete ==="
echo "Binary: install/freebsd/bin/ZepNotificationApp"