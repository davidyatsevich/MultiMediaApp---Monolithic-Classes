#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QVideoFrame>
#include <memory>
#include "SQLite.hpp"
#include "Recorders.hpp"

class NestedRecordingTabs : public QWidget
{
    Q_OBJECT

public:
    enum class Mode
    {
        Audio,
        Video
    };

    explicit NestedRecordingTabs(QWidget *parent, std::shared_ptr<SQLite> database,
                                 const QString &storageDirectory, Mode mode);
    ~NestedRecordingTabs();

    void initializeCamera(); // no-op in Audio mode

signals:
    void recordingSaved();

private slots:
    void startRecording();
    void stopRecording();
    void saveRecording();
    void onDurationChanged(qint64 duration);
    void onRecordingError(const QString &error);
    void updateVideoFrame(const QVideoFrame &frame); // Video mode only
    void onCameraInitialized(bool success);          // Video mode only

private:
    void setupUI();
    QString getFileExtension() const;
    QMediaFormat::FileFormat getFileFormat() const;
    QMediaFormat::AudioCodec getAudioCodec() const; // Audio mode only
    QMediaFormat::VideoCodec getVideoCodec() const; // Video mode only

    Mode m_mode;

    // Shared UI
    QPushButton *m_recordButton;
    QPushButton *m_stopButton;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
    QLabel *m_durationLabel;
    QLineEdit *m_fileNameEdit;
    QComboBox *m_formatSelector;

    // Video-only UI
    QLabel *m_previewLabel = nullptr;
    QComboBox *m_qualitySelector = nullptr;

    // Shared backend
    std::shared_ptr<SQLite> m_database;
    QString m_storageDirectory;
    QString m_currentRecordingPath;

    // Recorders — only one is non-null at a time
    AudioRecorder *m_audioRecorder = nullptr;
    VideoRecorder *m_videoRecorder = nullptr;
};