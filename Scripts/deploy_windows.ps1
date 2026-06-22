# deploy_windows.ps1
# Run this from a "Developer PowerShell for VS 2022" session so MSVC env vars are set.

$ErrorActionPreference = "Stop"

$ScriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir  = Resolve-Path (Join-Path $ScriptDir "..")
$AppName     = "MultiMediaApp"
$BuildDir    = Join-Path $ProjectDir "build"
$AppDir      = Join-Path $BuildDir $AppName
$InstallDir  = Join-Path $ProjectDir "Installation"

# --------------------------------------------------
# Detect Qt install
# --------------------------------------------------
$QtCandidates = @(
    "C:\Qt\6.11.1\msvc2022_64",
    "C:\Qt\6.11.0\msvc2022_64",
    "C:\Qt\6.10.2\msvc2022_64",
    "C:\Qt\6.11.1\msvc2022_arm64",
    "C:\Qt\6.11.0\msvc2022_arm64"
)

$Qt = $QtCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Qt) {
    Write-Error "Qt installation not found. Checked: $($QtCandidates -join ', ')"
    exit 1
}

Write-Host "=== Using Qt at: $Qt ==="

$WinDeployQt = Join-Path $Qt "bin\windeployqt.exe"
if (-not (Test-Path $WinDeployQt)) {
    Write-Error "windeployqt.exe not found at $WinDeployQt"
    exit 1
}

# --------------------------------------------------
# Locate the built binary
# --------------------------------------------------
$Binary = Join-Path $BuildDir "$AppName.exe"
if (-not (Test-Path $Binary)) {
    $Binary = Join-Path $BuildDir "Release\$AppName.exe"
}
if (-not (Test-Path $Binary)) {
    Write-Error "Could not find $AppName.exe under $BuildDir"
    exit 1
}

Write-Host "=== Creating app folder ==="
if (Test-Path $AppDir) { Remove-Item $AppDir -Recurse -Force }
New-Item -ItemType Directory -Path $AppDir | Out-Null
Copy-Item $Binary $AppDir

Write-Host "=== Copying runtime icons ==="
foreach ($icon in @("AppIconOn", "AppIconOff")) {
    $src = Join-Path $ProjectDir "Assets\$icon.png"
    if (Test-Path $src) {
        Copy-Item $src (Join-Path $AppDir "$icon.png")
        Write-Host "  Copied $icon.png"
    } else {
        Write-Warning "$icon.png not found at $src"
    }
}

Write-Host "=== Converting app icon to .ico ==="
$IconSrc = Join-Path $ProjectDir "Assets\AppIconOff.png"
$IcoPath = Join-Path $AppDir "AppIcon.ico"

$magick = Get-Command magick -ErrorAction SilentlyContinue
$convert = Get-Command convert -ErrorAction SilentlyContinue

if ((Test-Path $IconSrc) -and ($magick -or $convert)) {
    $tool = if ($magick) { "magick" } else { "convert" }
    & $tool $IconSrc -define icon:auto-resize=256,128,64,48,32,16 $IcoPath
    Write-Host "Icon converted to $IcoPath"
} else {
    Write-Warning "Skipping .ico generation (ImageMagick not found or icon missing)"
}

Write-Host "=== Running windeployqt ==="
& $WinDeployQt --release --no-translations --compiler-runtime (Join-Path $AppDir "$AppName.exe")

Write-Host "=== Copying multimedia backend plugins ==="
$MultimediaPluginDir = Join-Path $Qt "plugins\multimedia"
if (Test-Path $MultimediaPluginDir) {
    $destDir = Join-Path $AppDir "multimedia"
    New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    Copy-Item (Join-Path $MultimediaPluginDir "*.dll") $destDir -ErrorAction SilentlyContinue
}

Write-Host "=== Writing qt.conf ==="
@"
[Paths]
Plugins = .
"@ | Set-Content -Path (Join-Path $AppDir "qt.conf") -Encoding ASCII

New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null

# --------------------------------------------------
# Package: NSIS installer if available, else portable ZIP
# --------------------------------------------------
$MakeNsis = Get-Command makensis -ErrorAction SilentlyContinue
if (-not $MakeNsis) {
    # Common default install location, in case it's not on PATH
    $fallbackNsis = "${env:ProgramFiles(x86)}\NSIS\makensis.exe"
    if (Test-Path $fallbackNsis) { $MakeNsis = $fallbackNsis }
}

if ($MakeNsis) {
    Write-Host "=== Building NSIS installer ==="
    $NsiScript = Join-Path $BuildDir "$AppName.nsi"
    $SetupOut  = Join-Path $InstallDir "$AppName-Setup.exe"

    $nsiContent = @"
!define APP_NAME "$AppName"
!define APP_EXE "$AppName.exe"
!define INSTALL_DIR_NAME "MultiMediaApp"

Name "`${APP_NAME}"
OutFile "$SetupOut"
InstallDir "`$PROGRAMFILES64\`${INSTALL_DIR_NAME}"
RequestExecutionLevel admin

Page directory
Page instfiles

Section "Install"
    SetOutPath "`$INSTDIR"
    File /r "$AppDir\*.*"

    CreateDirectory "`$SMPROGRAMS\`${APP_NAME}"
    CreateShortcut "`$SMPROGRAMS\`${APP_NAME}\`${APP_NAME}.lnk" "`$INSTDIR\`${APP_EXE}"
    CreateShortcut "`$DESKTOP\`${APP_NAME}.lnk" "`$INSTDIR\`${APP_EXE}"

    WriteUninstaller "`$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "`$INSTDIR\*.*"
    RMDir /r "`$INSTDIR"
    Delete "`$SMPROGRAMS\`${APP_NAME}\`${APP_NAME}.lnk"
    Delete "`$DESKTOP\`${APP_NAME}.lnk"
    RMDir "`$SMPROGRAMS\`${APP_NAME}"
SectionEnd
"@

    Set-Content -Path $NsiScript -Value $nsiContent -Encoding ASCII
    & $MakeNsis $NsiScript
    Write-Host "Installer created: $SetupOut"
} else {
    Write-Host "=== NSIS not found, creating portable ZIP instead ==="
    $ZipPath = Join-Path $InstallDir "$AppName-windows-portable.zip"
    if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
    Compress-Archive -Path $AppDir -DestinationPath $ZipPath
    Write-Host "Portable ZIP created: $ZipPath"
}

Write-Host ""
Write-Host "=== Done (Windows) ==="
Write-Host "    App folder: $AppDir"
Write-Host "    Output:     $InstallDir"