#ifndef VIDEOPLAYERWINDOW_HPP
#define VIDEOPLAYERWINDOW_HPP

#include <QDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

class VideoPlayerWindow : public QDialog
{
    Q_OBJECT

public:
    explicit VideoPlayerWindow(const QString &filePath, const QString &fileName,
                               QWidget *parent = nullptr);
    ~VideoPlayerWindow();

private slots:
    void play();
    void pause();
    void stop();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderMoved(int position);
    void updateVideoFrame(const QVideoFrame &frame);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    void setupUI();
    QString formatTime(qint64 milliseconds) const;

    // NO MORE MANUAL CONVERSION METHODS NEEDED!
    // Qt's frame.toImage() handles everything

    // Media components
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QVideoSink *m_videoSink;

    // UI components
    QLabel *m_titleLabel;
    QLabel *m_videoLabel;
    QLabel *m_timeLabel;
    QSlider *m_timeline;
    QPushButton *m_playButton;
    QPushButton *m_pauseButton;
    QPushButton *m_stopButton;

    // File info
    QString m_filePath;
    QString m_fileName;
};

#endif // VIDEOPLAYERWINDOW_HPP