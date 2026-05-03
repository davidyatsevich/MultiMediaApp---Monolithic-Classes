#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "=== Cleaning build directory ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "=== Configuring ==="
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$HOME/Qt/6.11.0/macos

echo "=== Building ==="
cmake --build "$BUILD_DIR" --config Release -j$(sysctl -n hw.logicalcpu)

echo "=== Deploying ==="
bash "$PROJECT_DIR/scripts/deploy.sh"

echo "=== Opening ==="
open "$BUILD_DIR/MultiMediaApp.app"