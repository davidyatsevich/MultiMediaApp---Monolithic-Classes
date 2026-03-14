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

private:
    void setupUI();
    void requestPermissions();
    void ensureStorageDirectoryExists();
    QString getStorageDirectory() const;

    // Database and recordings
    std::shared_ptr<SQLite> m_database;
    std::vector<std::shared_ptr<File>> m_recordings;
    QString m_storageDirectory;

    // Main UI
    QWidget *m_centralWidget;
    QTabWidget *m_tabWidget;

    // Tabs
    MainTabs *m_mainTabs;

    friend class MainTabs;
};