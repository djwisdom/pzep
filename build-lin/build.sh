#!/bin/bash
# Build for Linux
# Run: chmod +x build.sh && ./build.sh

set -e

CONFIG=${1:-Release}
cd "$(dirname "$0")"

echo "=== Building for Linux ==="

# Configure
cmake -G "Unix Makefiles" \
    -DBUILD_QT=OFF \
    -DBUILD_IMGUI=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_DEMOS=OFF \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    ..

# Build tests
cmake --build . --target unittests --config "$CONFIG"

# Run notification tests
echo ""
echo "=== Running Notification Tests ==="
./tests/unittests --gtest_filter="Notification*"

echo ""
echo "=== Build Complete ==="
echo "Binary: tests/unittests"
echo "Platform: Linux"