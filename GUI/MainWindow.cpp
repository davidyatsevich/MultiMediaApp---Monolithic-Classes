#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QPermission>
#include <QDebug>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_database(std::make_shared<SQLite>(MainWindow::getDatabasePath()))
{
    ensureStorageDirectoryExists();
    setupUI();
}

std::string MainWindow::getDatabasePath()
{
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbDir);
    return (dbDir + "/MediaStorageImplicit.db").toStdString();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("Multi Media App");
    setMinimumSize(800, 600);
    resize(1000, 700);

    m_mainTabs = new MainTabs(this, m_database, m_storageDirectory);
    setCentralWidget(m_mainTabs);

    connect(m_mainTabs, &MainTabs::cameraPermissionRequested,
            this, &MainWindow::requestCameraPermission);
    connect(m_mainTabs, &MainTabs::microphonePermissionRequested,
            this, &MainWindow::requestMicrophonePermission);
}

void MainWindow::requestCameraPermission()
{
#if QT_CONFIG(permissions)
    QCameraPermission cameraPermission;
    auto status = qApp->checkPermission(cameraPermission);

    if (status == Qt::PermissionStatus::Granted)
    {
        m_mainTabs->onCameraPermissionResult(true);
        return;
    }

    if (status == Qt::PermissionStatus::Denied)
    {
        m_mainTabs->onCameraPermissionResult(false);
        return;
    }

    qApp->requestPermission(cameraPermission, this, [this](const QPermission &permission)
                            {
        bool granted = permission.status() == Qt::PermissionStatus::Granted;
        qDebug() << "Camera permission:" << (granted ? "GRANTED" : "DENIED");
        m_mainTabs->onCameraPermissionResult(granted); });
#else
    m_mainTabs->onCameraPermissionResult(true);
#endif
}

void MainWindow::requestMicrophonePermission()
{
#if QT_CONFIG(permissions)
    QMicrophonePermission micPermission;
    auto status = qApp->checkPermission(micPermission);

    if (status == Qt::PermissionStatus::Granted)
    {
        m_mainTabs->onMicrophonePermissionResult(true);
        return;
    }

    if (status == Qt::PermissionStatus::Denied)
    {
        m_mainTabs->onMicrophonePermissionResult(false);
        return;
    }

    qApp->requestPermission(micPermission, this, [this](const QPermission &permission)
                            {
        bool granted = permission.status() == Qt::PermissionStatus::Granted;
        qDebug() << "Microphone permission:" << (granted ? "GRANTED" : "DENIED");
        m_mainTabs->onMicrophonePermissionResult(granted); });
#else
    m_mainTabs->onMicrophonePermissionResult(true);
#endif
}

void MainWindow::ensureStorageDirectoryExists()
{
    m_storageDirectory = getStorageDirectory();
    QDir dir(m_storageDirectory);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            QMessageBox::critical(this, "Error",
                                  "Failed to create storage directory: " + m_storageDirectory);
        }
    }
}

QString MainWindow::getStorageDirectory() const
{
    QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/MediaStorageExplicit";
    QDir().mkpath(storageDir);
    return storageDir;
}