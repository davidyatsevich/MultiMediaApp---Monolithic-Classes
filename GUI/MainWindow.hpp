#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "SQLite.hpp"
#include "File.hpp"
#include "MainTabs.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void requestCameraPermission();
    void requestMicrophonePermission();

private:
    void setupUI();
    void ensureStorageDirectoryExists();
    QString getStorageDirectory() const;
    static std::string getDatabasePath();

    std::shared_ptr<SQLite> m_database;
    std::vector<std::shared_ptr<File>> m_recordings;
    QString m_storageDirectory;

    QWidget *m_centralWidget;
    QTabWidget *m_tabWidget;

    MainTabs *m_mainTabs;

    friend class MainTabs;
};