#!/bin/zsh
set -e

APP_NAME="MultiMediaApp"
BUILD_DIR="build"
APP_PATH="$BUILD_DIR/$APP_NAME.app"
ICON_PNG="icons/AppIcon.png"
IDENTITY="Local Qt Testing"

echo "==> Checking app bundle"
test -d "$APP_PATH"

echo "==> Running macdeployqt"
macdeployqt "$APP_PATH" -verbose=2

echo "==> Creating iconset"
ICONSET="AppIcon.iconset"
rm -rf "$ICONSET"
mkdir "$ICONSET"

sips -z 16 16     "$ICON_PNG" --out "$ICONSET/icon_16x16.png"
sips -z 32 32     "$ICON_PNG" --out "$ICONSET/icon_16x16@2x.png"
sips -z 32 32     "$ICON_PNG" --out "$ICONSET/icon_32x32.png"
sips -z 64 64     "$ICON_PNG" --out "$ICONSET/icon_32x32@2x.png"
sips -z 128 128   "$ICON_PNG" --out "$ICONSET/icon_128x128.png"
sips -z 256 256   "$ICON_PNG" --out "$ICONSET/icon_128x128@2x.png"
sips -z 256 256   "$ICON_PNG" --out "$ICONSET/icon_256x256.png"
sips -z 512 512   "$ICON_PNG" --out "$ICONSET/icon_256x256@2x.png"
sips -z 512 512   "$ICON_PNG" --out "$ICONSET/icon_512x512.png"
sips -z 1024 1024 "$ICON_PNG" --out "$ICONSET/icon_512x512@2x.png"

echo "==> Building icns"
iconutil -c icns "$ICONSET" -o AppIcon.icns

echo "==> Installing icon"
cp AppIcon.icns "$APP_PATH/Contents/Resources/"

echo "==> Code signing (local)"
codesign --force --deep --options runtime \
  --sign "$IDENTITY" "$APP_PATH"

echo "==> Verifying signature"
codesign --verify --deep --verbose "$APP_PATH"

echo "==> Gatekeeper check"
spctl --assess --verbose "$APP_PATH" || true

echo "==> Deployment finished"
