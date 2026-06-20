#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_NAME="MultiMediaApp"
BUILD_DIR="$PROJECT_DIR/build"
APPDIR="$BUILD_DIR/$APP_NAME"
INSTALL_DIR="$PROJECT_DIR/Installation"

# --------------------------------------------------
# Detect Qt install (Windows-style paths under MSYS2/Git Bash)
# --------------------------------------------------
if [ -d "$HOME/Qt/6.11.0/mingw_64" ]; then
    QT="$HOME/Qt/6.11.0/mingw_64"
elif [ -d "$HOME/Qt/6.10.2/mingw_64" ]; then
    QT="$HOME/Qt/6.10.2/mingw_64"
elif [ -d "$HOME/Qt/6.11.0/msvc2022_64" ]; then
    QT="$HOME/Qt/6.11.0/msvc2022_64"
elif [ -d "$HOME/Qt/6.10.2/msvc2022_64" ]; then
    QT="$HOME/Qt/6.10.2/msvc2022_64"
else
    echo "ERROR: Qt installation not found under \$HOME/Qt/<version>/mingw_64 or msvc2022_64"
    exit 1
fi

WINDEPLOYQT="$QT/bin/windeployqt.exe"
if [ ! -f "$WINDEPLOYQT" ]; then
    echo "ERROR: windeployqt.exe not found at $WINDEPLOYQT"
    exit 1
fi

echo "=== Using Qt at: $QT ==="

# --------------------------------------------------
# Locate the built binary
# --------------------------------------------------
BINARY="$BUILD_DIR/$APP_NAME.exe"
if [ ! -f "$BINARY" ]; then
    # Some generators (Ninja Multi-Config, VS) put it in a Release/ subfolder
    BINARY="$BUILD_DIR/Release/$APP_NAME.exe"
fi
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Could not find $APP_NAME.exe under $BUILD_DIR"
    exit 1
fi

echo "=== Creating app folder ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR"
cp "$BINARY" "$APPDIR/"

echo "=== Copying runtime icons ==="
for icon in AppIconOn AppIconOff; do
    SRC="$PROJECT_DIR/Assets/${icon}.png"
    if [ -f "$SRC" ]; then
        cp "$SRC" "$APPDIR/${icon}.png"
        echo "  Copied ${icon}.png"
    else
        echo "  WARNING: ${icon}.png not found at $SRC"
    fi
done

echo "=== Converting app icon to .ico (for the .exe and shortcuts) ==="
ICON_SRC="$PROJECT_DIR/Assets/AppIconOff.png"
ICO_PATH="$APPDIR/AppIcon.ico"

if [ -f "$ICON_SRC" ] && command -v convert &>/dev/null; then
    # ImageMagick can pack multiple resolutions into a single .ico
    convert "$ICON_SRC" -define icon:auto-resize=256,128,64,48,32,16 "$ICO_PATH"
    echo "Icon converted to $ICO_PATH"
else
    echo "WARNING: skipping .ico generation (ImageMagick 'convert' not found or icon missing)"
fi

echo "=== Running windeployqt ==="
# --release: pull release-mode runtime libs
# --no-translations: skip Qt's bundled translation files unless you need them
# --compiler-runtime: bundle MSVC/MinGW runtime DLLs so the target machine
#                     doesn't need a separate VC++ Redistributable installed
"$WINDEPLOYQT" \
    --release \
    --no-translations \
    --compiler-runtime \
    "$APPDIR/$APP_NAME.exe"

echo "=== Copying multimedia backend plugin (if not already pulled in) ==="
mkdir -p "$APPDIR/multimedia"
if [ -d "$QT/plugins/multimedia" ]; then
    cp "$QT/plugins/multimedia/"*.dll "$APPDIR/multimedia/" 2>/dev/null || true
fi

echo "=== Writing qt.conf ==="
cat > "$APPDIR/qt.conf" << 'EOF'
[Paths]
Plugins = .
EOF

# --------------------------------------------------
# Package: NSIS installer if available, else portable ZIP
# --------------------------------------------------
mkdir -p "$INSTALL_DIR"

MAKENSIS="$(command -v makensis || true)"
if [ -n "$MAKENSIS" ]; then
    echo "=== Building NSIS installer ==="
    NSI_SCRIPT="$BUILD_DIR/$APP_NAME.nsi"

    cat > "$NSI_SCRIPT" << EOF
!define APP_NAME "$APP_NAME"
!define APP_EXE "$APP_NAME.exe"
!define INSTALL_DIR_NAME "MultiMediaApp"

Name "\${APP_NAME}"
OutFile "$INSTALL_DIR/${APP_NAME}-Setup.exe"
InstallDir "\$PROGRAMFILES64\\\${INSTALL_DIR_NAME}"
RequestExecutionLevel admin

Page directory
Page instfiles

Section "Install"
    SetOutPath "\$INSTDIR"
    File /r "$APPDIR\\*.*"

    CreateDirectory "\$SMPROGRAMS\\\${APP_NAME}"
    CreateShortcut "\$SMPROGRAMS\\\${APP_NAME}\\\${APP_NAME}.lnk" "\$INSTDIR\\\${APP_EXE}"
    CreateShortcut "\$DESKTOP\\\${APP_NAME}.lnk" "\$INSTDIR\\\${APP_EXE}"

    WriteUninstaller "\$INSTDIR\\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "\$INSTDIR\\*.*"
    RMDir /r "\$INSTDIR"
    Delete "\$SMPROGRAMS\\\${APP_NAME}\\\${APP_NAME}.lnk"
    Delete "\$DESKTOP\\\${APP_NAME}.lnk"
    RMDir "\$SMPROGRAMS\\\${APP_NAME}"
SectionEnd
EOF

    "$MAKENSIS" "$NSI_SCRIPT"
    echo "Installer created: $INSTALL_DIR/${APP_NAME}-Setup.exe"
else
    echo "=== NSIS not found, creating portable ZIP instead ==="
    ZIP_PATH="$INSTALL_DIR/${APP_NAME}-windows-portable.zip"

    if command -v zip &>/dev/null; then
        (cd "$BUILD_DIR" && zip -r "$ZIP_PATH" "$APP_NAME")
        echo "Portable ZIP created: $ZIP_PATH"
    else
        echo "WARNING: 'zip' not found either. Install NSIS or zip to produce a package."
        echo "App folder is still available, unpackaged, at: $APPDIR"
    fi
fi

echo ""
echo "=== Done (Windows) ==="
echo "    App folder: $APPDIR"
echo "    Output:     $INSTALL_DIR/"