#include "Recorders.hpp"
#include <QMediaDevices>
#include <QUrl>
#include <QDebug>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent), m_isRecording(false)
{
    m_recorder = new QMediaRecorder(this);
    m_captureSession = new QMediaCaptureSession(this);
    m_audioInput = new QAudioInput(this);

    m_captureSession->setRecorder(m_recorder);
    m_captureSession->setAudioInput(m_audioInput);

    connect(m_recorder, &QMediaRecorder::durationChanged,
            this, &AudioRecorder::durationChanged);
    connect(m_recorder, &QMediaRecorder::errorOccurred,
            this, &AudioRecorder::onRecorderError);
}

AudioRecorder::~AudioRecorder()
{
    if (m_isRecording)
    {
        stopRecording();
    }
}

void AudioRecorder::startRecording(const QString &filePath,
                                   QMediaFormat::FileFormat format,
                                   QMediaFormat::AudioCodec codec)
{
    if (m_isRecording)
    {
        return;
    }

    m_recorder->setOutputLocation(QUrl::fromLocalFile(filePath));

    QMediaFormat mediaFormat;
    mediaFormat.setFileFormat(format);
    mediaFormat.setAudioCodec(codec);
    m_recorder->setMediaFormat(mediaFormat);

    m_recorder->setQuality(QMediaRecorder::HighQuality);
    m_recorder->record();

    m_isRecording = true;
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording)
    {
        return;
    }

    m_recorder->stop();
    m_isRecording = false;
    emit recordingStopped();
}

QString AudioRecorder::errorString() const
{
    return m_recorder->errorString();
}

void AudioRecorder::onRecorderError()
{
    m_isRecording = false;
    emit errorOccurred(m_recorder->errorString());
}

VideoRecorder::VideoRecorder(QObject *parent)
    : QObject(parent),
      m_recorder(nullptr),
      m_captureSession(nullptr),
      m_camera(nullptr),
      m_videoSink(nullptr),
      m_audioInput(nullptr),
      m_isRecording(false),
      m_cameraInitialized(false)
{
}

VideoRecorder::~VideoRecorder()
{
    if (m_isRecording)
    {
        stopRecording();
    }
    if (m_camera && m_camera->isActive())
    {
        m_camera->stop();
    }
}

bool VideoRecorder::initializeCamera()
{
    if (m_cameraInitialized)
    {
        return true;
    }

    qDebug() << "=== Initializing camera ===";

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    qDebug() << "Available cameras:" << cameras.size();

    if (cameras.isEmpty())
    {
        qDebug() << "ERROR: No cameras found!";
        emit cameraInitialized(false);
        return false;
    }

    for (const QCameraDevice &cam : cameras)
    {
        qDebug() << "  Camera:" << cam.description() << "Position:" << cam.position();
    }

    m_recorder = new QMediaRecorder(this);
    m_captureSession = new QMediaCaptureSession(this);
    m_camera = new QCamera(cameras.first(), this);
    m_videoSink = new QVideoSink(this);
    m_audioInput = new QAudioInput(this);

    m_captureSession->setCamera(m_camera);
    m_captureSession->setRecorder(m_recorder);
    m_captureSession->setVideoSink(m_videoSink);
    m_captureSession->setAudioInput(m_audioInput);

    connect(m_recorder, &QMediaRecorder::durationChanged,
            this, &VideoRecorder::durationChanged);
    connect(m_recorder, &QMediaRecorder::errorOccurred,
            this, &VideoRecorder::onRecorderError);
    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &VideoRecorder::onVideoFrameChanged);

    qDebug() << "Starting camera...";
    m_camera->start();

    qDebug() << "Camera active:" << m_camera->isActive();
    qDebug() << "Camera error:" << m_camera->error() << m_camera->errorString();

    m_cameraInitialized = true;
    emit cameraInitialized(true);

    qDebug() << "=== Camera initialization complete ===";
    return true;
}

void VideoRecorder::startRecording(const QString &filePath,
                                   QMediaFormat::FileFormat format,
                                   QMediaFormat::VideoCodec codec,
                                   QMediaRecorder::Quality quality)
{
    if (m_isRecording || !m_cameraInitialized)
    {
        return;
    }

    m_recorder->setOutputLocation(QUrl::fromLocalFile(filePath));

    QMediaFormat mediaFormat;
    mediaFormat.setFileFormat(format);
    mediaFormat.setVideoCodec(codec);
    mediaFormat.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_recorder->setMediaFormat(mediaFormat);

    m_recorder->setQuality(quality);
    m_recorder->record();

    m_isRecording = true;
}

void VideoRecorder::stopRecording()
{
    if (!m_isRecording)
    {
        return;
    }

    m_recorder->stop();
    m_isRecording = false;
    emit recordingStopped();
}

bool VideoRecorder::isCameraActive() const
{
    return m_camera && m_camera->isActive();
}

QString VideoRecorder::errorString() const
{
    if (m_recorder)
    {
        return m_recorder->errorString();
    }
    return QString();
}

void VideoRecorder::onRecorderError()
{
    m_isRecording = false;
    emit errorOccurred(m_recorder->errorString());
}

void VideoRecorder::onVideoFrameChanged(const QVideoFrame &frame)
{
    emit videoFrameChanged(frame);
}