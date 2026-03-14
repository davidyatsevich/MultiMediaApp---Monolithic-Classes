#pragma once
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QGroupBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>
#include <memory>
#include "SQLite.hpp"
#include "File.hpp"

class NestedEditingTabs : public QWidget
{
    Q_OBJECT

public:
    enum class Mode
    {
        Audio,
        Video
    };

    explicit NestedEditingTabs(QWidget *parent, std::shared_ptr<SQLite> database, Mode mode);
    ~NestedEditingTabs();

    void updateSelectors();

private slots:
    void onFileSelected(int index);
    void loadFile();
    void play();
    void pause();
    void trim();
    void extend();
    void save();
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);
    void onSliderMoved(int position);
    void updateVideoFrame(const QVideoFrame &frame); // Video mode only

private:
    void setupUI();
    void loadRecordings();

    Mode m_mode;

    // Shared UI
    QComboBox *m_fileSelector;
    QComboBox *m_extendSelector;
    QPushButton *m_loadButton;
    QPushButton *m_playButton;
    QPushButton *m_pauseButton;
    QPushButton *m_trimButton;
    QPushButton *m_extendButton;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
    QLabel *m_timeLabel;
    QLineEdit *m_trimStartEdit;
    QLineEdit *m_trimEndEdit;
    QLineEdit *m_outputNameEdit;
    QSlider *m_timeline;

    // Video-only UI
    QLabel *m_previewLabel = nullptr;

    // Shared media playback
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    // Video-only playback
    QVideoSink *m_videoSink = nullptr;

    // Data
    std::shared_ptr<SQLite> m_database;
    std::vector<std::shared_ptr<File>> m_recordings;
    QString m_currentFilePath;
};