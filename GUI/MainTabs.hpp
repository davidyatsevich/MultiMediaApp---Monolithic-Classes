#pragma once

#include <QWidget>
#include <QTabWidget>
#include <memory>
#include "SQLite.hpp"
#include "NestedRecordingTabs.hpp"
#include "NestedEditingTabs.hpp"
#include "File.hpp"
#include "VideoPlayerWindow.hpp"

#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTableWidget>

class MainTabs : public QTabWidget
{
    Q_OBJECT

public:
    explicit MainTabs(QWidget *parent, std::shared_ptr<SQLite> database,
                      const QString &storageDirectory);
    ~MainTabs();

public slots:
    void updateSelectors(); // formerly EditTab::updateSelectors + PlaybackTab::updateSelector
    void refreshTable();    // formerly StorageTab::refreshTable
    void initializeCamera();
signals:
    void recordingSaved();    // formerly RecordingTab::recordingSaved
    void recordingsChanged(); // formerly StorageTab::recordingsChanged

private slots:
    void playRecording();
    void pauseRecording();
    void addRecording();
    void editRecording();
    void deleteRecording();
    void cleanupStorage();
    void openStorageFolder();
    void importFromExplorer();
    void exportToExplorer();

private:
    void setupUI();
    void loadRecordings();

    // --- Recording tab ---
    QWidget *m_recordingTab;
    NestedRecordingTabs *m_audioRecordingTab;
    NestedRecordingTabs *m_videoRecordingTab;

    // --- Playback tab ---
    QWidget *m_playbackTab;
    QComboBox *m_recordingSelector;
    QPushButton *m_playButton;
    QPushButton *m_pauseButton;
    QSlider *m_timelineSlider;
    QLabel *m_statusLabel;
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    std::vector<std::shared_ptr<File>> m_recordings;

    // --- Edit tab ---
    QWidget *m_editTab;
    NestedEditingTabs *m_audioEditTab;
    NestedEditingTabs *m_videoEditTab;

    // --- Storage tab ---
    QWidget *m_storageTab;
    QLabel *m_titleLabel;
    QTableWidget *m_table;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_refreshButton;
    QPushButton *m_cleanupButton;
    QPushButton *m_openStorageButton;
    QPushButton *m_importButton;
    QPushButton *m_exportButton;

    // --- Shared data ---
    std::shared_ptr<SQLite> m_database;
    QString m_storageDirectory;
};