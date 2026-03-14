#include "VideoPlayerWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QPixmap>
#include <QImage>
#include <QVideoFrameFormat>

VideoPlayerWindow::VideoPlayerWindow(const QString &filePath, const QString &fileName,
                                     QWidget *parent)
    : QDialog(parent), m_filePath(filePath), m_fileName(fileName)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_videoSink = new QVideoSink(this);

    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoSink(m_videoSink);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &VideoPlayerWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &VideoPlayerWindow::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &VideoPlayerWindow::onPlaybackStateChanged);
    connect(m_videoSink, &QVideoSink::videoFrameChanged,
            this, &VideoPlayerWindow::updateVideoFrame);

    setupUI();

    // Load and play video
    m_player->setSource(QUrl::fromLocalFile(m_filePath));
    m_player->play();
}

VideoPlayerWindow::~VideoPlayerWindow()
{
    if (m_player)
    {
        m_player->stop();
    }
}

void VideoPlayerWindow::setupUI()
{
    setWindowTitle("Video Player - " + m_fileName);
    setMinimumSize(800, 600);
    resize(960, 720);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Title
    m_titleLabel = new QLabel(m_fileName);
    m_titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // Video display
    m_videoLabel = new QLabel;
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setMinimumSize(640, 480);
    m_videoLabel->setStyleSheet(
        "QLabel { background-color: black; border: 2px solid #333; }");
    m_videoLabel->setScaledContents(false);
    mainLayout->addWidget(m_videoLabel, 1);

    // Timeline
    m_timeline = new QSlider(Qt::Horizontal);
    connect(m_timeline, &QSlider::sliderMoved, this, &VideoPlayerWindow::onSliderMoved);
    mainLayout->addWidget(m_timeline);

    // Time label
    m_timeLabel = new QLabel("00:00:00 / 00:00:00");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("font-size: 11pt;");
    mainLayout->addWidget(m_timeLabel);

    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    m_playButton = new QPushButton("▶ Play");
    m_playButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-size: 12pt; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #45a049; }");
    connect(m_playButton, &QPushButton::clicked, this, &VideoPlayerWindow::play);
    buttonLayout->addWidget(m_playButton);

    m_pauseButton = new QPushButton("⏸ Pause");
    m_pauseButton->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; font-size: 12pt; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #e68900; }");
    m_pauseButton->setEnabled(false);
    connect(m_pauseButton, &QPushButton::clicked, this, &VideoPlayerWindow::pause);
    buttonLayout->addWidget(m_pauseButton);

    m_stopButton = new QPushButton("⏹ Stop");
    m_stopButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; font-size: 12pt; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #da190b; }");
    connect(m_stopButton, &QPushButton::clicked, this, &VideoPlayerWindow::stop);
    buttonLayout->addWidget(m_stopButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
}

void VideoPlayerWindow::play()
{
    m_player->play();
}

void VideoPlayerWindow::pause()
{
    m_player->pause();
}

void VideoPlayerWindow::stop()
{
    m_player->stop();
    m_timeline->setValue(0);
    m_videoLabel->clear();
    m_videoLabel->setText("Stopped");
    m_videoLabel->setStyleSheet(
        "QLabel { background-color: black; color: white; border: 2px solid #333; font-size: 16pt; }");
}

void VideoPlayerWindow::onPositionChanged(qint64 position)
{
    if (!m_timeline->isSliderDown())
    {
        m_timeline->setValue(position);
    }

    QString currentTime = formatTime(position);
    QString duration = formatTime(m_player->duration());
    m_timeLabel->setText(currentTime + " / " + duration);
}

void VideoPlayerWindow::onDurationChanged(qint64 duration)
{
    m_timeline->setMaximum(duration);
    QString durationStr = formatTime(duration);
    m_timeLabel->setText("00:00:00 / " + durationStr);
}

void VideoPlayerWindow::onSliderMoved(int position)
{
    m_player->setPosition(position);
}

void VideoPlayerWindow::updateVideoFrame(const QVideoFrame &frame)
{
    if (!m_videoLabel)
        return;

    // Create a working copy of the frame
    QVideoFrame cloneFrame(frame);

    // Map the frame for reading
    if (!cloneFrame.map(QVideoFrame::ReadOnly))
        return;

    // Use Qt's built-in hardware-accelerated conversion (handles NV12, YUV420P, etc.)
    QImage img = cloneFrame.toImage();

    // Unmap immediately after conversion
    cloneFrame.unmap();

    // Check if conversion succeeded
    if (img.isNull())
        return;

    // Convert to RGB32 for display if needed
    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
    {
        img = img.convertToFormat(QImage::Format_RGB32);
    }

    // Scale to fit label while maintaining aspect ratio
    QPixmap pixmap = QPixmap::fromImage(img);
    QPixmap scaled = pixmap.scaled(m_videoLabel->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    m_videoLabel->setPixmap(scaled);
}

void VideoPlayerWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state)
    {
    case QMediaPlayer::PlayingState:
        m_playButton->setEnabled(false);
        m_pauseButton->setEnabled(true);
        break;
    case QMediaPlayer::PausedState:
        m_playButton->setEnabled(true);
        m_pauseButton->setEnabled(false);
        break;
    case QMediaPlayer::StoppedState:
        m_playButton->setEnabled(true);
        m_pauseButton->setEnabled(false);
        break;
    }
}

QString VideoPlayerWindow::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes % 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}