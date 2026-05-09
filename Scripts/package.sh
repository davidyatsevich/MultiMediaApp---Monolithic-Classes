#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PLATFORM="$(uname -s)"
ARCH="$(uname -m)"

echo "=== Cleaning build directory ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "=== Configuring ==="
if [ "$PLATFORM" = "Darwin" ]; then
    CMAKE_PREFIX="$HOME/Qt/6.11.0/macos"
else
    CMAKE_PREFIX="$HOME/Qt/6.10.2/gcc_64"
fi

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX"

echo "=== Building ==="
if [ "$PLATFORM" = "Darwin" ]; then
    cmake --build "$BUILD_DIR" --config Release -j$(sysctl -n hw.logicalcpu)
else
    cmake --build "$BUILD_DIR" --config Release -j$(nproc)
fi

echo "=== Deploying ==="
if [ "$PLATFORM" = "Darwin" ]; then
    bash "$PROJECT_DIR/Scripts/deploy_macos.sh"
else
    bash "$PROJECT_DIR/Scripts/deploy_linux.sh"
fi

echo ""
echo "=== Build Complete ==="
echo "    Platform: $PLATFORM ($ARCH)"
echo "    Output:   $PROJECT_DIR/Installation/"