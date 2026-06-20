#include "MainWindow.hpp"
#include <QApplication>
#include <QDebug>
#include <QIcon>

#ifdef __APPLE__
#include <QtPlugin>

QT_BEGIN_NAMESPACE
class QDarwinCameraPermissionPlugin;
class QDarwinMicrophonePermissionPlugin;
QT_END_NAMESPACE

Q_IMPORT_PLUGIN(QDarwinCameraPermissionPlugin)
Q_IMPORT_PLUGIN(QDarwinMicrophonePermissionPlugin)

// Cocoa bridge to set the Dock icon
#include <unistd.h>
#include <objc/objc.h>
#include <objc/message.h>

static void setDockIcon(const QString &imagePath)
{
    // NSImage *img = [NSImage alloc] initWithContentsOfFile: imagePath]
    id cls = (id)objc_getClass("NSImage");
    id path = ((id (*)(id, SEL, void *))objc_msgSend)(
        (id)objc_getClass("NSString"), sel_getUid("stringWithUTF8String:"),
        (void *)imagePath.toUtf8().constData());
    id img = ((id (*)(id, SEL))objc_msgSend)(cls, sel_getUid("alloc"));
    img = ((id (*)(id, SEL, id))objc_msgSend)(img, sel_getUid("initWithContentsOfFile:"), path);

    // [NSApplication sharedApplication].applicationIconImage = img
    id app = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"),
                                             sel_getUid("sharedApplication"));
    ((void (*)(id, SEL, id))objc_msgSend)(app, sel_getUid("setApplicationIconImage:"), img);
}
#endif

#ifdef __linux__
#include <QFile>
#include <QTextStream>

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
            return line.mid(static_cast<int>(qstrlen("PRETTY_NAME="))).remove(QLatin1Char('"'));
    }
    return QStringLiteral("Linux (unknown distro)");
}
#endif

#ifdef _WIN32
#include <QSysInfo>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#if defined(__APPLE__)
    qDebug("Platform : macOS");
    qDebug("Backend  : AVFoundation");

    // Resolve icon paths relative to the bundle Resources folder
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/../Resources";
    QString iconOn = resourcesPath + "/AppIconOn.png";
    QString iconOff = resourcesPath + "/AppIconOff.png";

    // Switch to "on" icon immediately
    setDockIcon(iconOn);

    // Restore "off" icon when the app quits
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [iconOff]()
                     {
        setDockIcon(iconOff);
        // Give the Dock a moment to render before the process exits
        usleep(120000); });

#elif defined(__linux__)
    const QString distro = detectDistro();
    qDebug("Platform : %s", qPrintable(distro));
    qDebug("Backend  : GStreamer (V4L2 + PulseAudio/PipeWire)");

    // On Linux, use QApplication::setWindowIcon (taskbar / window icon)
    QString iconOn = QCoreApplication::applicationDirPath() + "/AppIconOn.png";
    app.setWindowIcon(QIcon(iconOn));
    // No explicit restore needed — process exit clears it automatically
#elif defined(_WIN32)
    qDebug("Platform : Windows (%s)", qPrintable(QSysInfo::prettyProductName()));
    qDebug("Backend  : WMF (Windows Media Foundation)");

    // Windows has no Dock; the taskbar icon is set per-window via setWindowIcon.
    // Unlike macOS, there's no separate "running" vs "not running" Dock state to
    // restore on quit — the OS simply stops showing the icon when the process exits.
    const QString iconOn = QCoreApplication::applicationDirPath() + "/AppIconOn.png";
    app.setWindowIcon(QIcon(iconOn));
#endif

    MainWindow window;
    window.show();
    return app.exec();
}