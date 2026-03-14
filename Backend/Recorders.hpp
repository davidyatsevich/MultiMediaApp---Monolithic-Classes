#pragma once
#include <QObject>
#include <QMediaRecorder>
#include <QMediaCaptureSession>
#include <QCamera>
#include <QVideoSink>
#include <QAudioInput>
#include <QMediaFormat>
#include <QVideoFrame>
#include <QString>

class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    void startRecording(const QString &filePath, QMediaFormat::FileFormat format,
                        QMediaFormat::AudioCodec codec);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    QString errorString() const;

signals:
    void durationChanged(qint64 duration);
    void errorOccurred(const QString &error);
    void recordingStopped();

private slots:
    void onRecorderError();

private:
    QMediaRecorder *m_recorder;
    QMediaCaptureSession *m_captureSession;
    QAudioInput *m_audioInput;
    bool m_isRecording;
};
class VideoRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VideoRecorder(QObject *parent = nullptr);
    ~VideoRecorder();

    bool initializeCamera();
    void startRecording(const QString &filePath, QMediaFormat::FileFormat format,
                        QMediaFormat::VideoCodec codec, QMediaRecorder::Quality quality);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }
    bool isCameraActive() const;
    QString errorString() const;
    QVideoSink *videoSink() const { return m_videoSink; }

signals:
    void durationChanged(qint64 duration);
    void errorOccurred(const QString &error);
    void recordingStopped();
    void videoFrameChanged(const QVideoFrame &frame);
    void cameraInitialized(bool success);

private slots:
    void onRecorderError();
    void onVideoFrameChanged(const QVideoFrame &frame);

private:
    QMediaRecorder *m_recorder;
    QMediaCaptureSession *m_captureSession;
    QCamera *m_camera;
    QVideoSink *m_videoSink;
    QAudioInput *m_audioInput;
    bool m_isRecording;
    bool m_cameraInitialized;
};