#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_NAME="MultiMediaApp"
BUILD_DIR="$PROJECT_DIR/build"
ARCH="$(uname -m)"
APPDIR="$BUILD_DIR/AppDir"
INSTALL_DIR="$PROJECT_DIR/Installation"

# Detect Qt path
if [ -d "$HOME/Qt/6.10.2/gcc_64" ]; then
    QT="$HOME/Qt/6.10.2/gcc_64"
elif [ -d "$HOME/Qt/6.11.0/gcc_64" ]; then
    QT="$HOME/Qt/6.11.0/gcc_64"
else
    echo "ERROR: Qt installation not found"
    exit 1
fi

# Detect appimagetool
if [ "$ARCH" = "aarch64" ]; then
    APPIMAGETOOL="$PROJECT_DIR/appimagetool-aarch64.AppImage"
else
    APPIMAGETOOL="$PROJECT_DIR/appimagetool-x86_64.AppImage"
fi

echo "=== Creating AppDir structure ==="
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/plugins/platforms"
mkdir -p "$APPDIR/usr/plugins/sqldrivers"
mkdir -p "$APPDIR/usr/plugins/imageformats"
mkdir -p "$APPDIR/usr/plugins/multimedia"

echo "=== Copying binary ==="
cp "$BUILD_DIR/$APP_NAME" "$APPDIR/usr/bin/"

echo "=== Copying Qt libraries ==="
for lib in QtCore QtGui QtWidgets QtMultimedia QtMultimediaWidgets QtSql QtNetwork QtOpenGL QtDBus QtConcurrent QtOpenGLWidgets QtXcbQpa; do
    cp -r "$QT/lib/lib$lib"*.so* "$APPDIR/usr/lib/" 2>/dev/null || true
done

echo "=== Copying Qt plugins ==="
cp "$QT/plugins/platforms/libqxcb.so"          "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
cp "$QT/plugins/sqldrivers/libqsqlite.so"      "$APPDIR/usr/plugins/sqldrivers/" 2>/dev/null || true
cp "$QT/plugins/imageformats/"*.so             "$APPDIR/usr/plugins/imageformats/" 2>/dev/null || true
cp "$QT/plugins/multimedia/"*.so               "$APPDIR/usr/plugins/multimedia/" 2>/dev/null || true

echo "=== Creating desktop entry ==="
cat > "$APPDIR/$APP_NAME.desktop" << EOF
[Desktop Entry]
Type=Application
Name=MultiMedia App
Exec=$APP_NAME
Icon=icon
Categories=AudioVideo;
EOF
cp "$APPDIR/$APP_NAME.desktop" "$APPDIR/usr/share/applications/"

echo "=== Copying icon ==="
if [ -f "$PROJECT_DIR/Assets/icon.png" ]; then
    cp "$PROJECT_DIR/Assets/icon.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/icon.png"
    cp "$PROJECT_DIR/Assets/icon.png" "$APPDIR/icon.png"
else
    echo "No icon found, skipping"
fi

echo "=== Creating AppRun ==="
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
exec "${HERE}/usr/bin/MultiMediaApp" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "=== Building AppImage ==="
if [ -f "$APPIMAGETOOL" ]; then
    "$APPIMAGETOOL" "$APPDIR" "$BUILD_DIR/$APP_NAME-$ARCH.AppImage"
    mkdir -p "$INSTALL_DIR"
    cp "$BUILD_DIR/$APP_NAME-$ARCH.AppImage" "$INSTALL_DIR/"
    echo "=== Done (Linux) ==="
    echo "    AppImage: $INSTALL_DIR/$APP_NAME-$ARCH.AppImage"
else
    echo "WARNING: appimagetool not found at $APPIMAGETOOL"
    echo "Running directly: $BUILD_DIR/$APP_NAME"
fi