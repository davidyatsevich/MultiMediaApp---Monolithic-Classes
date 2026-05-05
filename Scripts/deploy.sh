#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="MultiMediaApp"
BUILD_DIR="/Users/davidyatsevich/Desktop/Personal Projects/Side-Portfolio C++/MultiMediaApp - Monolithic Classes/build"
PROJECT_DIR="/Users/davidyatsevich/Desktop/Personal Projects/Side-Portfolio C++/MultiMediaApp - Monolithic Classes"
INSTALL_DIR="$PROJECT_DIR/Installation"

APP="/Users/davidyatsevich/Desktop/Personal Projects/Side-Portfolio C++/MultiMediaApp - Monolithic Classes/build/MultiMediaApp.app"
QT="$HOME/Qt/6.11.0/macos"


echo "=== Creating bundle directories ==="
mkdir -p "$APP/Contents/Frameworks"
mkdir -p "$APP/Contents/PlugIns/platforms"
mkdir -p "$APP/Contents/PlugIns/styles"
mkdir -p "$APP/Contents/PlugIns/sqldrivers"
mkdir -p "$APP/Contents/PlugIns/imageformats"
mkdir -p "$APP/Contents/PlugIns/mediaservice"
mkdir -p "$APP/Contents/Resources"

echo "=== Copying Qt frameworks ==="
for fw in QtCore QtGui QtWidgets QtMultimedia QtMultimediaWidgets QtSql QtNetwork QtOpenGL QtDBus QtConcurrent QtOpenGLWidgets; do
    cp -R "$QT/lib/$fw.framework" "$APP/Contents/Frameworks/"
done

echo "=== Copying plugins ==="
cp "$QT/plugins/platforms/libqcocoa.dylib"     "$APP/Contents/PlugIns/platforms/"
cp "$QT/plugins/styles/libqmacstyle.dylib"     "$APP/Contents/PlugIns/styles/"
cp "$QT/plugins/sqldrivers/libqsqlite.dylib"   "$APP/Contents/PlugIns/sqldrivers/"
cp "$QT/plugins/imageformats/libqjpeg.dylib"   "$APP/Contents/PlugIns/imageformats/"
cp "$QT/plugins/imageformats/libqgif.dylib"    "$APP/Contents/PlugIns/imageformats/"
cp "$QT/plugins/imageformats/libqtiff.dylib"   "$APP/Contents/PlugIns/imageformats/"

echo "=== Copying multimedia plugins ==="
mkdir -p "$APP/Contents/PlugIns/multimedia"
cp "$QT/plugins/multimedia/libdarwinmediaplugin.dylib"  "$APP/Contents/PlugIns/multimedia/"
cp "$QT/plugins/multimedia/libffmpegmediaplugin.dylib"  "$APP/Contents/PlugIns/multimedia/"

echo "=== Writing qt.conf ==="
cat > "$APP/Contents/Resources/qt.conf" << 'EOF'
[Paths]
Plugins = PlugIns
Frameworks = Frameworks
EOF

echo "=== Fixing Info.plist ==="
cat > "$APP/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>MultiMediaApp</string>
    <key>CFBundleDisplayName</key>
    <string>MultiMedia App</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.multimediaapp</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleExecutable</key>
    <string>MultiMediaApp</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>NSCameraUsageDescription</key>
    <string>This app needs access to your camera to record videos.</string>
    <key>NSMicrophoneUsageDescription</key>
    <string>This app needs access to your microphone to record audio and video with sound.</string>
</dict>
</plist>
EOF

echo "=== Fixing rpaths in main binary ==="
BINARY="$APP/Contents/MacOS/MultiMediaApp"
for fw in QtCore QtGui QtWidgets QtMultimedia QtMultimediaWidgets QtSql QtNetwork QtOpenGL QtDBus; do
    install_name_tool -change \
        "$QT/lib/$fw.framework/Versions/A/$fw" \
        "@executable_path/../Frameworks/$fw.framework/Versions/A/$fw" \
        "$BINARY"
done

echo "=== Fixing bundle rpath ==="
install_name_tool -delete_rpath \
    "$HOME/Qt/6.11.0/macos/lib" \
    "$BINARY" 2>/dev/null || true

install_name_tool -add_rpath \
    "@executable_path/../Frameworks" \
    "$BINARY" 2>/dev/null || true


echo "=== Converting icon ==="
ICON_SRC="$SCRIPT_DIR/../Assets/icon.png"
ICONSET_DIR="$SCRIPT_DIR/../Assets/icon.iconset"
ICNS_PATH="$APP/Contents/Resources/AppIcon.icns"

if [ -f "$ICON_SRC" ]; then
    mkdir -p "$ICONSET_DIR"
    sips -z 16 16     "$ICON_SRC" --out "$ICONSET_DIR/icon_16x16.png"
    sips -z 32 32     "$ICON_SRC" --out "$ICONSET_DIR/icon_16x16@2x.png"
    sips -z 32 32     "$ICON_SRC" --out "$ICONSET_DIR/icon_32x32.png"
    sips -z 64 64     "$ICON_SRC" --out "$ICONSET_DIR/icon_32x32@2x.png"
    sips -z 128 128   "$ICON_SRC" --out "$ICONSET_DIR/icon_128x128.png"
    sips -z 256 256   "$ICON_SRC" --out "$ICONSET_DIR/icon_128x128@2x.png"
    sips -z 256 256   "$ICON_SRC" --out "$ICONSET_DIR/icon_256x256.png"
    sips -z 512 512   "$ICON_SRC" --out "$ICONSET_DIR/icon_256x256@2x.png"
    sips -z 512 512   "$ICON_SRC" --out "$ICONSET_DIR/icon_512x512.png"
    sips -z 1024 1024 "$ICON_SRC" --out "$ICONSET_DIR/icon_512x512@2x.png"
    iconutil -c icns "$ICONSET_DIR" -o "$ICNS_PATH"
    rm -rf "$ICONSET_DIR"
    echo "Icon converted and copied to bundle"
else
    echo "No icon found at $ICON_SRC, skipping"
fi

echo "=== Signing ==="
codesign --deep --force --sign - "$APP"

echo "=== Creating DMG ==="
DMG_PATH="$BUILD_DIR/$APP_NAME.dmg"
DMG_STAGING="$BUILD_DIR/dmg_staging"

# Create staging directory
mkdir -p "$DMG_STAGING"
cp -R "$APP" "$DMG_STAGING/"

# Create symlink to Applications folder
ln -sf /Applications "$DMG_STAGING/Applications"

# Create DMG
hdiutil create \
    -volname "$APP_NAME" \
    -srcfolder "$DMG_STAGING" \
    -ov \
    -format UDZO \
    "$DMG_PATH"

# Cleanup staging
rm -rf "$DMG_STAGING"

echo "=== Copying to Installation folder ==="
INSTALL_DIR="$PROJECT_DIR/Installation"
mkdir -p "$INSTALL_DIR"
cp "$DMG_PATH" "$INSTALL_DIR/"
echo "=== Installation folder: $INSTALL_DIR ==="

echo "=== DMG created at: $DMG_PATH ==="

echo "=== Done ==="