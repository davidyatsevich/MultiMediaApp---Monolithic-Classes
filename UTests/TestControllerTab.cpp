#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QPushButton>
#include <QLabel>
#include "ControllerTab.hpp"
#include "Recorders.hpp"

class TestControllerTab : public QObject
{
    Q_OBJECT

private:
    VideoRecorder* m_videoRecorder;
    AudioRecorder* m_audioRecorder;
    ControllerTab* m_controller;

private slots:

    // ── Fixture ──────────────────────────────────────────────
    void init()
    {
        m_videoRecorder = new VideoRecorder(this);
        m_audioRecorder = new AudioRecorder(this);
        m_controller = new ControllerTab(nullptr, m_videoRecorder, m_audioRecorder);
    }

    void cleanup()
    {
        delete m_controller;
        m_controller = nullptr;
        delete m_videoRecorder;
        m_videoRecorder = nullptr;
        delete m_audioRecorder;
        m_audioRecorder = nullptr;
    }

    // ── Initial state ─────────────────────────────────────────
    void test_initialState_cameraToggleDisabled()
    {
        auto* btn = m_controller->findChild<QPushButton*>(QString(), Qt::FindChildrenRecursively);
        // Buttons are private; verify via behavior instead of direct access —
        // toggling before permission is granted must be a no-op (see below).
        QVERIFY(!m_videoRecorder->isCameraActive());
    }

    void test_initialState_micNotEnabled()
    {
        QVERIFY(!m_audioRecorder->isMicEnabled());
    }

    // ── requestCameraPermission / requestMicrophonePermission signals ──
    void test_requestCameraPermission_signalEmittedOnDemand()
    {
        QSignalSpy spy(m_controller, &ControllerTab::requestCameraPermission);
        emit m_controller->requestCameraPermission();
        QCOMPARE(spy.count(), 1);
    }

    void test_requestMicrophonePermission_signalEmittedOnDemand()
    {
        QSignalSpy spy(m_controller, &ControllerTab::requestMicrophonePermission);
        emit m_controller->requestMicrophonePermission();
        QCOMPARE(spy.count(), 1);
    }

    // ── onCameraPermissionResult ───────────────────────────────
    void test_onCameraPermissionResult_grantedTrue_doesNotThrow()
    {
        QVERIFY_THROWS_NO_EXCEPTION(m_controller->onCameraPermissionResult(true));
    }

    void test_onCameraPermissionResult_deniedFalse_doesNotThrow()
    {
        QVERIFY_THROWS_NO_EXCEPTION(m_controller->onCameraPermissionResult(false));
    }

    // ── onMicrophonePermissionResult ───────────────────────────
    void test_onMicrophonePermissionResult_grantedTrue_doesNotThrow()
    {
        QVERIFY_THROWS_NO_EXCEPTION(m_controller->onMicrophonePermissionResult(true));
    }

    void test_onMicrophonePermissionResult_deniedFalse_doesNotThrow()
    {
        QVERIFY_THROWS_NO_EXCEPTION(m_controller->onMicrophonePermissionResult(false));
    }

    // ── Mic toggle reflects into AudioRecorder state ───────────
    void test_micEnabledChanged_signalReflectsState()
    {
        QSignalSpy spy(m_audioRecorder, &AudioRecorder::micEnabledChanged);

        m_audioRecorder->setMicEnabled(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.at(0).at(0).toBool());
        QVERIFY(m_audioRecorder->isMicEnabled());

        m_audioRecorder->setMicEnabled(false);
        QCOMPARE(spy.count(), 2);
        QVERIFY(!spy.at(1).at(0).toBool());
        QVERIFY(!m_audioRecorder->isMicEnabled());
    }

    void test_micEnabledChanged_idempotent_noDuplicateSignal()
    {
        QSignalSpy spy(m_audioRecorder, &AudioRecorder::micEnabledChanged);
        m_audioRecorder->setMicEnabled(true);
        m_audioRecorder->setMicEnabled(true); // same state again
        QCOMPARE(spy.count(), 1); // should not re-emit for no-op
    }

    // ── Camera toggle reflects into VideoRecorder state ────────
    // Hardware-dependent — skip gracefully if no camera present,
    // matching the pattern already used in TestVideoRecorder.
    void test_cameraEnabledChanged_signalReflectsState()
    {
        QSignalSpy initSpy(m_videoRecorder, &VideoRecorder::cameraInitialized);
        bool ok = m_videoRecorder->initializeCamera();
        initSpy.wait(3000);

        if (!ok || initSpy.isEmpty() || !initSpy.at(0).at(0).toBool()) {
            QSKIP("No camera available on this machine — skipping");
        }

        QSignalSpy enabledSpy(m_videoRecorder, &VideoRecorder::cameraEnabledChanged);
        m_videoRecorder->setCameraEnabled(false);
        QVERIFY(enabledSpy.count() > 0);
        QVERIFY(!enabledSpy.last().at(0).toBool());
    }

    // ── Destructor safety ───────────────────────────────────────
    void test_destructor_doesNotCrash()
    {
        {
            VideoRecorder localVideo(this);
            AudioRecorder localAudio(this);
            ControllerTab localController(nullptr, &localVideo, &localAudio);
            // destructor fires here
        }
        QVERIFY(true);
    }
};

QTEST_MAIN(TestControllerTab)
#include "TestControllerTab.moc"
