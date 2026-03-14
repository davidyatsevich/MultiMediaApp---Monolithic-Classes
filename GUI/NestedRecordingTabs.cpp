#include "NestedRecordingTabs.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QImage>
#include <QVideoFrameFormat>

NestedRecordingTabs::NestedRecordingTabs(QWidget *parent, std::shared_ptr<SQLite> database,
                                         const QString &storageDirectory, Mode mode)
    : QWidget(parent), m_database(database), m_storageDirectory(storageDirectory), m_mode(mode)
{
    if (m_mode == Mode::Audio)
    {
        m_audioRecorder = new AudioRecorder(this);
        connect(m_audioRecorder, &AudioRecorder::durationChanged,
                this, &NestedRecordingTabs::onDurationChanged);
        connect(m_audioRecorder, &AudioRecorder::errorOccurred,
                this, &NestedRecordingTabs::onRecordingError);
    }
    else
    {
        m_videoRecorder = new VideoRecorder(this);
        connect(m_videoRecorder, &VideoRecorder::durationChanged,
                this, &NestedRecordingTabs::onDurationChanged);
        connect(m_videoRecorder, &VideoRecorder::errorOccurred,
                this, &NestedRecordingTabs::onRecordingError);
        connect(m_videoRecorder, &VideoRecorder::videoFrameChanged,
                this, &NestedRecordingTabs::updateVideoFrame);
        connect(m_videoRecorder, &VideoRecorder::cameraInitialized,
                this, &NestedRecordingTabs::onCameraInitialized);
    }

    setupUI();
}

NestedRecordingTabs::~NestedRecordingTabs()
{
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void NestedRecordingTabs::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Video-only: camera preview
    if (m_mode == Mode::Video)
    {
        QGroupBox *previewGroup = new QGroupBox("Camera Preview");
        QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

        m_previewLabel = new QLabel("Waiting for camera permission...");
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setMinimumSize(640, 480);
        m_previewLabel->setMaximumSize(640, 480);
        m_previewLabel->setStyleSheet(
            "QLabel { background-color: black; color: white; border: 2px solid #ccc; font-size: 16pt; }");
        m_previewLabel->setScaledContents(false);
        previewLayout->addWidget(m_previewLabel);

        layout->addWidget(previewGroup);
    }

    // Shared: status group
    QGroupBox *statusGroup = new QGroupBox("Recording Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);

    m_statusLabel = new QLabel("Ready to record");
    m_statusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: green;");
    statusLayout->addWidget(m_statusLabel);

    m_durationLabel = new QLabel("Duration: 00:00:00");
    m_durationLabel->setStyleSheet("font-size: 12pt;");
    statusLayout->addWidget(m_durationLabel);

    layout->addWidget(statusGroup);

    // Shared: file/details group
    QString groupTitle = (m_mode == Mode::Audio) ? "Recording Details" : "Video Details";
    QGroupBox *fileGroup = new QGroupBox(groupTitle);
    QFormLayout *fileLayout = new QFormLayout(fileGroup);

    m_fileNameEdit = new QLineEdit;
    m_fileNameEdit->setPlaceholderText(m_mode == Mode::Audio
                                           ? "Enter recording name (optional)"
                                           : "Enter video name (optional)");
    fileLayout->addRow("File Name:", m_fileNameEdit);

    m_formatSelector = new QComboBox;
    if (m_mode == Mode::Audio)
    {
        m_formatSelector->addItem("M4A (AAC) - Recommended", 0);
        m_formatSelector->addItem("MP3 (MPEG Audio)", 1);
        m_formatSelector->addItem("OGG (Vorbis)", 2);
        m_formatSelector->addItem("FLAC (Lossless)", 3);
        m_formatSelector->addItem("WAV (Uncompressed)", 4);
    }
    else
    {
        m_formatSelector->addItem("MP4 (H.264) - Recommended", 0);
        m_formatSelector->addItem("MOV (QuickTime)", 1);
        m_formatSelector->addItem("MPEG", 2);
        m_formatSelector->addItem("AVI", 3);
    }
    fileLayout->addRow("Format:", m_formatSelector);

    // Video-only: quality selector
    if (m_mode == Mode::Video)
    {
        m_qualitySelector = new QComboBox;
        m_qualitySelector->addItem("High Quality", QMediaRecorder::HighQuality);
        m_qualitySelector->addItem("Normal Quality", QMediaRecorder::NormalQuality);
        m_qualitySelector->addItem("Low Quality", QMediaRecorder::LowQuality);
        m_qualitySelector->setCurrentIndex(0);
        fileLayout->addRow("Quality:", m_qualitySelector);
    }

    layout->addWidget(fileGroup);

    // Shared: control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    m_recordButton = new QPushButton("Start Recording");
    m_recordButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-size: 14pt; padding: 10px; }"
        "QPushButton:hover { background-color: #45a049; }");
    connect(m_recordButton, &QPushButton::clicked, this, &NestedRecordingTabs::startRecording);
    buttonLayout->addWidget(m_recordButton);

    m_stopButton = new QPushButton("Stop Recording");
    m_stopButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; font-size: 14pt; padding: 10px; }"
        "QPushButton:hover { background-color: #da190b; }");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &NestedRecordingTabs::stopRecording);
    buttonLayout->addWidget(m_stopButton);

    m_saveButton = new QPushButton("Save Recording");
    m_saveButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-size: 14pt; padding: 10px; }"
        "QPushButton:hover { background-color: #0b7dda; }");
    m_saveButton->setEnabled(false);
    connect(m_saveButton, &QPushButton::clicked, this, &NestedRecordingTabs::saveRecording);
    buttonLayout->addWidget(m_saveButton);

    layout->addLayout(buttonLayout);
    layout->addStretch();
}

void NestedRecordingTabs::initializeCamera()
{
    if (m_mode == Mode::Video)
        m_videoRecorder->initializeCamera();
}

// ─── Recording control ────────────────────────────────────────────────────────

void NestedRecordingTabs::startRecording()
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString extension = getFileExtension();
    QString prefix = (m_mode == Mode::Audio) ? "recording" : "video";
    QString fileName = m_fileNameEdit->text().isEmpty()
                           ? QString("%1_%2.%3").arg(prefix, timestamp, extension)
                           : QString("%1_%2.%3").arg(m_fileNameEdit->text(), timestamp, extension);

    m_currentRecordingPath = QDir(m_storageDirectory).filePath(fileName);

    if (m_mode == Mode::Audio)
    {
        m_audioRecorder->startRecording(m_currentRecordingPath, getFileFormat(), getAudioCodec());
    }
    else
    {
        QMediaRecorder::Quality quality = static_cast<QMediaRecorder::Quality>(
            m_qualitySelector->currentData().toInt());
        m_videoRecorder->startRecording(m_currentRecordingPath, getFileFormat(), getVideoCodec(), quality);
    }

    m_recordButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_saveButton->setEnabled(false);
    m_statusLabel->setText("Recording...");
    m_statusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: red;");
}

void NestedRecordingTabs::stopRecording()
{
    if (m_mode == Mode::Audio)
        m_audioRecorder->stopRecording();
    else
        m_videoRecorder->stopRecording();

    m_recordButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_saveButton->setEnabled(true);
    m_statusLabel->setText("Recording stopped");
    m_statusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: orange;");
}

void NestedRecordingTabs::saveRecording()
{
    if (m_currentRecordingPath.isEmpty())
    {
        QMessageBox::warning(this, "No Recording",
                             m_mode == Mode::Audio ? "No recording to save."
                                                   : "No video recording to save.");
        return;
    }

    QFileInfo fileInfo(m_currentRecordingPath);
    QString fileName = m_fileNameEdit->text().isEmpty()
                           ? fileInfo.fileName()
                           : m_fileNameEdit->text() + "." + fileInfo.suffix();

    try
    {
        m_database->insertRecording(
            m_currentRecordingPath.toStdString(),
            fileInfo.suffix().toUpper().toStdString(),
            fileName.toStdString());

        QMessageBox::information(this, "Success",
                                 m_mode == Mode::Audio ? "Recording saved successfully!"
                                                       : "Video recording saved successfully!");

        m_saveButton->setEnabled(false);
        m_statusLabel->setText(m_mode == Mode::Audio ? "Recording saved" : "Video saved");
        m_statusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: green;");
        m_fileNameEdit->clear();
        m_currentRecordingPath.clear();

        emit recordingSaved();
    }
    catch (const std::exception &e)
    {
        QMessageBox::warning(this, "Error",
                             QString("Failed to save %1: %2")
                                 .arg(m_mode == Mode::Audio ? "recording" : "video")
                                 .arg(e.what()));
    }
}

// ─── Shared slots ─────────────────────────────────────────────────────────────

void NestedRecordingTabs::onDurationChanged(qint64 duration)
{
    qint64 seconds = duration / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    m_durationLabel->setText(QString("Duration: %1:%2:%3")
                                 .arg(hours, 2, 10, QChar('0'))
                                 .arg(minutes % 60, 2, 10, QChar('0'))
                                 .arg(seconds % 60, 2, 10, QChar('0')));
}

void NestedRecordingTabs::onRecordingError(const QString &error)
{
    QString title = (m_mode == Mode::Audio) ? "Recording Error" : "Video Recording Error";
    QMessageBox::critical(this, title, QString("An error occurred: %1").arg(error));
    stopRecording();
}

// ─── Video-only slots ─────────────────────────────────────────────────────────

void NestedRecordingTabs::updateVideoFrame(const QVideoFrame &frame)
{
    if (!m_previewLabel)
        return;

    static int skipCounter = 0;
    if (++skipCounter % 2 != 0)
        return;

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QVideoFrame::ReadOnly))
        return;

    QImage img = cloneFrame.toImage();
    cloneFrame.unmap();

    if (img.isNull())
        return;

    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_RGB32);

    QPixmap pixmap = QPixmap::fromImage(img);
    QPixmap scaled = pixmap.scaled(m_previewLabel->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaled);
    m_previewLabel->setText("");
}

void NestedRecordingTabs::onCameraInitialized(bool success)
{
    if (!success)
    {
        m_previewLabel->setText("Camera initialization failed");
        m_previewLabel->setStyleSheet(
            "QLabel { background-color: black; color: red; border: 2px solid #ccc; font-size: 16pt; }");
    }
}

// ─── Format helpers ───────────────────────────────────────────────────────────

QMediaFormat::FileFormat NestedRecordingTabs::getFileFormat() const
{
    int index = m_formatSelector->currentData().toInt();
    if (m_mode == Mode::Audio)
    {
        switch (index)
        {
        case 0:
            return QMediaFormat::MPEG4;
        case 1:
            return QMediaFormat::MP3;
        case 2:
            return QMediaFormat::Ogg;
        case 3:
            return QMediaFormat::FLAC;
        case 4:
            return QMediaFormat::Wave;
        default:
            return QMediaFormat::MPEG4;
        }
    }
    else
    {
        switch (index)
        {
        case 0:
            return QMediaFormat::MPEG4;
        case 1:
            return QMediaFormat::QuickTime;
        case 2:
            return QMediaFormat::MPEG4;
        case 3:
            return QMediaFormat::AVI;
        default:
            return QMediaFormat::MPEG4;
        }
    }
}

QMediaFormat::AudioCodec NestedRecordingTabs::getAudioCodec() const
{
    int index = m_formatSelector->currentData().toInt();
    switch (index)
    {
    case 0:
        return QMediaFormat::AudioCodec::AAC;
    case 1:
        return QMediaFormat::AudioCodec::MP3;
    case 2:
        return QMediaFormat::AudioCodec::Vorbis;
    case 3:
        return QMediaFormat::AudioCodec::FLAC;
    case 4:
        return QMediaFormat::AudioCodec::Wave;
    default:
        return QMediaFormat::AudioCodec::AAC;
    }
}

QMediaFormat::VideoCodec NestedRecordingTabs::getVideoCodec() const
{
    // All video formats currently use H264
    return QMediaFormat::VideoCodec::H264;
}

QString NestedRecordingTabs::getFileExtension() const
{
    int index = m_formatSelector->currentData().toInt();
    if (m_mode == Mode::Audio)
    {
        switch (index)
        {
        case 0:
            return "m4a";
        case 1:
            return "mp3";
        case 2:
            return "ogg";
        case 3:
            return "flac";
        case 4:
            return "wav";
        default:
            return "m4a";
        }
    }
    else
    {
        switch (index)
        {
        case 0:
            return "mp4";
        case 1:
            return "mov";
        case 2:
            return "mpg";
        case 3:
            return "avi";
        default:
            return "mp4";
        }
    }
}