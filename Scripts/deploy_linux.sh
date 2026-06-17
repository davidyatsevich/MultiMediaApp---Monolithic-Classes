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
if [ -d "$HOME/Qt/6.10.2/gcc_arm64" ]; then
    QT="$HOME/Qt/6.10.2/gcc_arm64"
elif [ -d "$HOME/Qt/6.11.0/gcc_arm64" ]; then
    QT="$HOME/Qt/6.11.0/gcc_arm64"
elif [ -d "$HOME/Qt/6.10.2/gcc_64" ]; then
    QT="$HOME/Qt/6.10.2/gcc_64"
elif [ -d "$HOME/Qt/6.11.0/gcc_64" ]; then
    QT="$HOME/Qt/6.11.0/gcc_64"
else
    echo "ERROR: Qt installation not found"
    exit 1
fi

# Detect appimagetool
APPIMAGETOOL="$HOME/.local/bin/appimagetool"

echo "=== Creating AppDir structure ==="
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/128x128/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/64x64/apps"
mkdir -p "$APPDIR/usr/plugins/platforms"
mkdir -p "$APPDIR/usr/plugins/sqldrivers"
mkdir -p "$APPDIR/usr/plugins/imageformats"
mkdir -p "$APPDIR/usr/plugins/multimedia"

echo "=== Copying binary ==="
cp "$BUILD_DIR/$APP_NAME" "$APPDIR/usr/bin/"

echo "=== Copying runtime icon (taskbar on-state) ==="
if [ -f "$PROJECT_DIR/Assets/AppIconOn.png" ]; then
    cp "$PROJECT_DIR/Assets/AppIconOn.png" "$APPDIR/usr/bin/AppIconOn.png"
    echo "Copied AppIconOn.png next to binary"
else
    echo "WARNING: AppIconOn.png not found, taskbar icon will fall back to default"
fi

echo "=== Copying Qt libraries ==="
for lib in QtCore QtGui QtWidgets QtMultimedia QtMultimediaWidgets QtSql QtNetwork QtOpenGL QtDBus QtConcurrent QtOpenGLWidgets QtXcbQpa; do
    cp -r "$QT/lib/lib$lib"*.so* "$APPDIR/usr/lib/" 2>/dev/null || true
done

echo "=== Copying Qt plugins ==="
cp "$QT/plugins/platforms/libqxcb.so"      "$APPDIR/usr/plugins/platforms/" 2>/dev/null || true
cp "$QT/plugins/sqldrivers/libqsqlite.so"  "$APPDIR/usr/plugins/sqldrivers/" 2>/dev/null || true
cp "$QT/plugins/imageformats/"*.so         "$APPDIR/usr/plugins/imageformats/" 2>/dev/null || true
cp "$QT/plugins/multimedia/"*.so           "$APPDIR/usr/plugins/multimedia/" 2>/dev/null || true

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
if [ -f "$PROJECT_DIR/Assets/AppIconOff.png" ]; then
    cp "$PROJECT_DIR/Assets/AppIconOff.png" "$APPDIR/icon.png"
    cp "$PROJECT_DIR/Assets/AppIconOff.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/icon.png"
    if command -v convert &>/dev/null; then
        convert "$PROJECT_DIR/Assets/AppIconOff.png" -resize 128x128 \
            "$APPDIR/usr/share/icons/hicolor/128x128/apps/icon.png" 2>/dev/null || true
        convert "$PROJECT_DIR/Assets/AppIconOff.png" -resize 64x64 \
            "$APPDIR/usr/share/icons/hicolor/64x64/apps/icon.png" 2>/dev/null || true
    fi
    echo "Icon copied"
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
    echo "AppImage built: $BUILD_DIR/$APP_NAME-$ARCH.AppImage"
else
    echo "WARNING: appimagetool not found at $APPIMAGETOOL, skipping AppImage"
fi

echo "=== Creating RPM ==="
RPM_ROOT="$BUILD_DIR/rpm_build"
mkdir -p "$RPM_ROOT"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

cat > "$RPM_ROOT/SPECS/$APP_NAME.spec" << EOF
Name:           $APP_NAME
Version:        1.0.0
Release:        1%{?dist}
Summary:        MultiMedia recording and playback application
License:        Proprietary
AutoReqProv:    no

%description
A native multimedia application for recording and playing audio and video.

%install
mkdir -p %{buildroot}/opt/$APP_NAME
mkdir -p %{buildroot}/usr/local/bin
mkdir -p %{buildroot}/usr/share/applications
mkdir -p %{buildroot}/usr/share/icons/hicolor/256x256/apps

cp "$BUILD_DIR/$APP_NAME-$ARCH.AppImage" %{buildroot}/opt/$APP_NAME/$APP_NAME.AppImage
chmod +x %{buildroot}/opt/$APP_NAME/$APP_NAME.AppImage

cat > %{buildroot}/usr/local/bin/$APP_NAME << 'LAUNCHER'
#!/bin/bash
exec /opt/$APP_NAME/$APP_NAME.AppImage "\$@"
LAUNCHER
chmod +x %{buildroot}/usr/local/bin/$APP_NAME

cat > %{buildroot}/usr/share/applications/$APP_NAME.desktop << 'DESKTOP'
[Desktop Entry]
Type=Application
Name=MultiMedia App
Exec=$APP_NAME
Icon=$APP_NAME
Categories=AudioVideo;
DESKTOP

if [ -f "$PROJECT_DIR/Assets/AppIconOff.png" ]; then
    cp "$PROJECT_DIR/Assets/AppIconOff.png" %{buildroot}/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png
fi

%files
/opt/$APP_NAME/$APP_NAME.AppImage
/usr/local/bin/$APP_NAME
/usr/share/applications/$APP_NAME.desktop
/usr/share/icons/hicolor/256x256/apps/$APP_NAME.png

%post
update-desktop-database /usr/share/applications || true
gtk-update-icon-cache /usr/share/icons/hicolor || true

%preun
true

%postun
update-desktop-database /usr/share/applications || true
EOF

rpmbuild --define "_topdir $RPM_ROOT" \
         --define "_arch $ARCH" \
         -bb "$RPM_ROOT/SPECS/$APP_NAME.spec"

echo "=== Copying RPM to Installation ==="
mkdir -p "$INSTALL_DIR"
RPM_FILE=$(find "$RPM_ROOT/RPMS" -name "*.rpm" | head -1)
if [ -n "$RPM_FILE" ]; then
    cp "$RPM_FILE" "$INSTALL_DIR/"
    echo "RPM copied: $INSTALL_DIR/$(basename $RPM_FILE)"
else
    echo "WARNING: No RPM found to copy"
fi

echo ""
echo "=== Done (Linux) ==="
echo "    AppImage: $BUILD_DIR/$APP_NAME-$ARCH.AppImage"
echo "    RPM:      $INSTALL_DIR/"


