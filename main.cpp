
#include "MainWindow.hpp"
#include <QApplication>
#include <QDebug>

// ============================================================
// Platform-specific includes
// ============================================================

// ── macOS ────────────────────────────────────────────────────
// Import Qt's static permission plugins so the OS camera/mic
// permission dialogs are wired up correctly.
// These symbols only exist in the macOS Qt build; including
// them on Linux produces an "undefined reference" linker error.
#ifdef __APPLE__
#include <QtPlugin>

QT_BEGIN_NAMESPACE
class QDarwinCameraPermissionPlugin;
class QDarwinMicrophonePermissionPlugin;
QT_END_NAMESPACE

Q_IMPORT_PLUGIN(QDarwinCameraPermissionPlugin)
Q_IMPORT_PLUGIN(QDarwinMicrophonePermissionPlugin)
#endif

// ── Linux (Ubuntu / Fedora / any distro) ─────────────────────
// Camera (V4L2) and microphone (PulseAudio or PipeWire) access
// is granted at the OS level on Linux — no in-app permission
// plugin is needed.  Qt6 Multimedia discovers devices through
// GStreamer automatically at runtime.
// We include QFile/QTextStream only to read /etc/os-release for
// a runtime informational log message; they are not required for
// functionality.
#ifdef __linux__
#include <QFile>
#include <QTextStream>
#endif

// ============================================================
// Helpers
// ============================================================

#ifdef __linux__
// Read PRETTY_NAME from /etc/os-release to distinguish Ubuntu,
// Fedora, Arch, etc. at runtime.  The compiled binary is
// identical across distros; only the required packages differ.
static QString detectDistro()
{
    QFile f("/etc/os-release");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringLiteral("Linux (unknown distro)");

    QTextStream in(&f);
    while (!in.atEnd())
    {
        const QString line = in.readLine();
        if (line.startsWith(QLatin1String("PRETTY_NAME=")))
        {
            return line.mid(static_cast<int>(
                                qstrlen("PRETTY_NAME=")))
                .remove(QLatin1Char('"'));
        }
    }
    return QStringLiteral("Linux (unknown distro)");
}
#endif

// ============================================================
// main
// ============================================================
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ── Platform log ─────────────────────────────────────────
#if defined(__APPLE__)
    qDebug("Platform : macOS");
    qDebug("Plugins  : QDarwinCameraPermissionPlugin, "
           "QDarwinMicrophonePermissionPlugin (statically linked)");
    qDebug("Backend  : AVFoundation");

#elif defined(__linux__)
    const QString distro = detectDistro();

    // Detect Ubuntu/Debian vs Fedora/RHEL at runtime
    const bool isDebian = distro.contains(QLatin1String("Ubuntu"), Qt::CaseInsensitive) || distro.contains(QLatin1String("Debian"), Qt::CaseInsensitive) || distro.contains(QLatin1String("Mint"), Qt::CaseInsensitive) || distro.contains(QLatin1String("Pop!_OS"), Qt::CaseInsensitive);

    const bool isFedora = distro.contains(QLatin1String("Fedora"), Qt::CaseInsensitive) || distro.contains(QLatin1String("RHEL"), Qt::CaseInsensitive) || distro.contains(QLatin1String("CentOS"), Qt::CaseInsensitive) || distro.contains(QLatin1String("Rocky"), Qt::CaseInsensitive) || distro.contains(QLatin1String("Alma"), Qt::CaseInsensitive);

    qDebug("Platform : %s", qPrintable(distro));
    qDebug("Plugins  : none required (camera/mic granted by OS)");
    qDebug("Backend  : GStreamer (V4L2 + PulseAudio/PipeWire)");

    if (isDebian)
    {
        qDebug("Distro   : Debian/Ubuntu family — dependencies installed via apt");
    }
    else if (isFedora)
    {
        qDebug("Distro   : Fedora/RHEL family — dependencies installed via dnf");
    }
    else
    {
        qDebug("Distro   : Other Linux — install Qt6 + GStreamer via your package manager");
    }

#else
    // Fallback for any other platform (Windows, FreeBSD, …)
    qDebug("Platform : Unknown — no platform-specific configuration applied");
#endif

    // ── Launch ───────────────────────────────────────────────
    MainWindow window;
    window.show();
    return app.exec();
}
