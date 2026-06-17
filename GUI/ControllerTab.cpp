#include "ControllerTab.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

ControllerTab::ControllerTab(QWidget *parent, VideoRecorder *videoRecorder,
                             AudioRecorder *audioRecorder)
    : QWidget(parent), m_videoRecorder(videoRecorder), m_audioRecorder(audioRecorder)
{
    setupUI();

    connect(m_videoRecorder, &VideoRecorder::cameraEnabledChanged,
            this, &ControllerTab::onCameraEnabledChanged);
    connect(m_audioRecorder, &AudioRecorder::micEnabledChanged,
            this, &ControllerTab::onMicEnabledChanged);
}

ControllerTab::~ControllerTab()
{
}

void ControllerTab::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QGroupBox *permGroup = new QGroupBox("Permissions");
    QHBoxLayout *permLayout = new QHBoxLayout(permGroup);

    m_requestCameraButton = new QPushButton("Request Camera Access");
    m_requestCameraButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
    connect(m_requestCameraButton, &QPushButton::clicked,
            this, &ControllerTab::requestCameraPermission);
    permLayout->addWidget(m_requestCameraButton);

    m_requestMicButton = new QPushButton("Request Microphone Access");
    m_requestMicButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
    connect(m_requestMicButton, &QPushButton::clicked,
            this, &ControllerTab::requestMicrophonePermission);
    permLayout->addWidget(m_requestMicButton);

    layout->addWidget(permGroup);

    QGroupBox *deviceGroup = new QGroupBox("Devices");
    QHBoxLayout *deviceLayout = new QHBoxLayout(deviceGroup);

    QVBoxLayout *cameraColumn = new QVBoxLayout;
    m_cameraToggleButton = new QPushButton("Turn Camera On");
    m_cameraToggleButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    m_cameraToggleButton->setEnabled(false);
    connect(m_cameraToggleButton, &QPushButton::clicked,
            this, &ControllerTab::toggleCamera);
    cameraColumn->addWidget(m_cameraToggleButton);

    m_cameraStatusLabel = new QLabel("Camera: off");
    cameraColumn->addWidget(m_cameraStatusLabel);
    deviceLayout->addLayout(cameraColumn);

    QVBoxLayout *micColumn = new QVBoxLayout;
    m_micToggleButton = new QPushButton("Turn Mic On");
    m_micToggleButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    m_micToggleButton->setEnabled(false);
    connect(m_micToggleButton, &QPushButton::clicked,
            this, &ControllerTab::toggleMic);
    micColumn->addWidget(m_micToggleButton);

    m_micStatusLabel = new QLabel("Mic: off");
    micColumn->addWidget(m_micStatusLabel);
    deviceLayout->addLayout(micColumn);

    layout->addWidget(deviceGroup);
    layout->addStretch();
}

void ControllerTab::onCameraPermissionResult(bool granted)
{
    m_cameraPermissionGranted = granted;
    updateButtonStates();
}

void ControllerTab::onMicrophonePermissionResult(bool granted)
{
    m_micPermissionGranted = granted;
    updateButtonStates();
}

void ControllerTab::updateButtonStates()
{
    m_cameraToggleButton->setEnabled(m_cameraPermissionGranted);
    m_micToggleButton->setEnabled(m_micPermissionGranted);
}

void ControllerTab::toggleCamera()
{
    bool currentlyActive = m_videoRecorder->isCameraActive();
    if (!currentlyActive)
        m_videoRecorder->initializeCamera(); // no-op if already initialized

    m_videoRecorder->setCameraEnabled(!currentlyActive);
}

void ControllerTab::toggleMic()
{
    m_audioRecorder->setMicEnabled(!m_audioRecorder->isMicEnabled());
}

void ControllerTab::onCameraEnabledChanged(bool enabled)
{
    m_cameraToggleButton->setText(enabled ? "Turn Camera Off" : "Turn Camera On");
    m_cameraStatusLabel->setText(enabled ? "Camera: on" : "Camera: off");
}

void ControllerTab::onMicEnabledChanged(bool enabled)
{
    m_micToggleButton->setText(enabled ? "Turn Mic Off" : "Turn Mic On");
    m_micStatusLabel->setText(enabled ? "Mic: on" : "Mic: off");
}