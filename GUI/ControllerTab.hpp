#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include "Recorders.hpp"

class ControllerTab : public QWidget
{
    Q_OBJECT

public:
    explicit ControllerTab(QWidget *parent, VideoRecorder *videoRecorder,
                           AudioRecorder *audioRecorder);
    ~ControllerTab();

signals:
    void requestCameraPermission();
    void requestMicrophonePermission();

public slots:
    void onCameraPermissionResult(bool granted);
    void onMicrophonePermissionResult(bool granted);

private slots:
    void toggleCamera();
    void toggleMic();
    void onCameraEnabledChanged(bool enabled);
    void onMicEnabledChanged(bool enabled);

private:
    void setupUI();
    void updateButtonStates();

    VideoRecorder *m_videoRecorder;
    AudioRecorder *m_audioRecorder;

    QPushButton *m_requestCameraButton;
    QPushButton *m_requestMicButton;
    QPushButton *m_cameraToggleButton;
    QPushButton *m_micToggleButton;
    QLabel *m_cameraStatusLabel;
    QLabel *m_micStatusLabel;

    bool m_cameraPermissionGranted = false;
    bool m_micPermissionGranted = false;
};