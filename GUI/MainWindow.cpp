#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QPermission>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_database(std::make_shared<SQLite>("MediaStorageImplicit.db"))
{
    ensureStorageDirectoryExists();
    setupUI();
    requestPermissions();
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
}

void MainWindow::requestPermissions()
{
#if QT_CONFIG(permissions)
    QCameraPermission cameraPermission;
    QMicrophonePermission micPermission;

    auto cameraStatus = qApp->checkPermission(cameraPermission);
    auto micStatus = qApp->checkPermission(micPermission);

    qDebug() << "Camera permission status:" << (int)cameraStatus;
    qDebug() << "Microphone permission status:" << (int)micStatus;

    if (cameraStatus == Qt::PermissionStatus::Undetermined)
    {
        qDebug() << "Requesting camera permission...";
        qApp->requestPermission(cameraPermission, this, [this, micPermission](const QPermission &permission)
                                {
            if (permission.status() == Qt::PermissionStatus::Granted)
            {
                qDebug() << "Camera GRANTED!";
                qApp->requestPermission(micPermission, this, [this](const QPermission &p)
                {
                    if (p.status() == Qt::PermissionStatus::Granted)
                    {
                        qDebug() << "Microphone GRANTED!";
                        m_mainTabs->initializeCamera();
                    }
                    else
                    {
                        qDebug() << "Microphone DENIED!";
                    }
                });
            }
            else
            {
                qDebug() << "Camera DENIED!";
            } });
    }
    else if (cameraStatus == Qt::PermissionStatus::Granted)
    {
        qDebug() << "Camera already granted";
        if (micStatus == Qt::PermissionStatus::Undetermined)
        {
            qDebug() << "Requesting microphone permission...";
            qApp->requestPermission(micPermission, this, [this](const QPermission &p)
                                    {
                if (p.status() == Qt::PermissionStatus::Granted)
                {
                    qDebug() << "Microphone GRANTED!";
                    m_mainTabs->initializeCamera();
                }
                else
                {
                    qDebug() << "Microphone DENIED!";
                } });
        }
        else if (micStatus == Qt::PermissionStatus::Granted)
        {
            qDebug() << "Both permissions already granted!";
            m_mainTabs->initializeCamera();
        }
    }
#else
    m_recordingTab->initializeCamera();
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
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    dir.cdUp();
    QString storagePath = dir.filePath("Backend/MediaStorageExplicit");
    return storagePath;
}