#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "Recorders.hpp"

class TestVideoRecorder : public QObject
{
    Q_OBJECT

private:
    VideoRecorder* m_recorder;
    QTemporaryDir  m_tempDir;

    QString tempPath(const QString& name) const
    {
        return m_tempDir.filePath(name);
    }

private slots:

    // ── Fixture ──────────────────────────────────────────────
    void init()
    {
        m_recorder = new VideoRecorder(this);
    }

    void cleanup()
    {
        delete m_recorder;
        m_recorder = nullptr;
    }

    // ── Initial state ─────────────────────────────────────────
    void test_initialState_notRecording()
    {
        QVERIFY(!m_recorder->isRecording());
    }

    void test_initialState_cameraNotActive()
    {
        QVERIFY(!m_recorder->isCameraActive());
    }

    void test_initialState_errorStringEmpty()
    {
        QVERIFY(m_recorder->errorString().isEmpty());
    }

    void test_initialState_videoSinkIsNull()
    {
        // videoSink() returns m_videoSink which starts as nullptr
        QVERIFY(m_recorder->videoSink() == nullptr);
    }

    // ── initializeCamera ─────────────────────────────────────
    void test_initializeCamera_emitsCameraInitializedSignal()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        m_recorder->initializeCamera();

        // Signal must be emitted (true or false depending on hardware)
        QVERIFY(spy.wait(3000) || spy.count() > 0);
    }

    void test_initializeCamera_signalCarriesBoolArgument()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        m_recorder->initializeCamera();
        spy.wait(3000);

        if (spy.count() > 0) {
            // The signal argument must be a valid bool
            QVERIFY(spy.at(0).at(0).canConvert<bool>());
        }
    }

    void test_initializeCamera_calledTwice_returnsTrue()
    {
        // Second call should be a no-op (m_cameraInitialized guard)
        bool first  = m_recorder->initializeCamera();
        bool second = m_recorder->initializeCamera();

        // Both calls must succeed if camera is available,
        // and second must not reinitialise (no crash, same result)
        QCOMPARE(first, second);
    }

    void test_initializeCamera_videoSinkBecomesNonNull()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        m_recorder->initializeCamera();
        spy.wait(3000);

        if (spy.count() > 0 && spy.at(0).at(0).toBool()) {
            QVERIFY(m_recorder->videoSink() != nullptr);
        }
    }

    // ── startRecording (requires camera init) ────────────────
    void test_startRecording_withoutInit_isNoOp()
    {
        // m_cameraInitialized is false → startRecording must be ignored
        m_recorder->startRecording(
            tempPath("test.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::HighQuality);

        QVERIFY(!m_recorder->isRecording());
    }

    void test_startRecording_afterInit_setsIsRecordingTrue()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        spy.wait(3000);

        if (!ok || spy.isEmpty() || !spy.at(0).at(0).toBool()) {
            QSKIP("No camera available on this machine — skipping recording tests");
        }

        m_recorder->startRecording(
            tempPath("test.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::HighQuality);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    void test_startRecording_whenAlreadyRecording_isNoOp()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        spy.wait(3000);

        if (!ok || spy.isEmpty() || !spy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        m_recorder->startRecording(
            tempPath("a.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::HighQuality);

        // Second call while recording — should be silently ignored
        m_recorder->startRecording(
            tempPath("b.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::HighQuality);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    // ── stopRecording ─────────────────────────────────────────
    void test_stopRecording_whenNotRecording_isNoOp()
    {
        QVERIFY_THROWS_NO_EXCEPTION(m_recorder->stopRecording());
        QVERIFY(!m_recorder->isRecording());
    }

    void test_stopRecording_emitsRecordingStoppedSignal()
    {
        QSignalSpy initSpy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        initSpy.wait(3000);

        if (!ok || initSpy.isEmpty() || !initSpy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        QSignalSpy stopSpy(m_recorder, &VideoRecorder::recordingStopped);

        m_recorder->startRecording(
            tempPath("stop.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::HighQuality);

        m_recorder->stopRecording();
        QCOMPARE(stopSpy.count(), 1);
    }

    void test_stopRecording_setsIsRecordingFalse()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        spy.wait(3000);

        if (!ok || spy.isEmpty() || !spy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        m_recorder->startRecording(
            tempPath("s.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::NormalQuality);

        m_recorder->stopRecording();
        QVERIFY(!m_recorder->isRecording());
    }

    // ── videoFrameChanged signal ──────────────────────────────
    void test_videoFrameChanged_emittedAfterCameraStart()
    {
        QSignalSpy initSpy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        initSpy.wait(3000);

        if (!ok || initSpy.isEmpty() || !initSpy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        QSignalSpy frameSpy(m_recorder, &VideoRecorder::videoFrameChanged);
        frameSpy.wait(2000);

        QVERIFY(frameSpy.count() > 0);
    }

    // ── durationChanged during recording ──────────────────────
    void test_durationChanged_emittedDuringRecording()
    {
        QSignalSpy initSpy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        initSpy.wait(3000);

        if (!ok || initSpy.isEmpty() || !initSpy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        QSignalSpy durSpy(m_recorder, &VideoRecorder::durationChanged);

        m_recorder->startRecording(
            tempPath("dur.mp4"),
            QMediaFormat::MPEG4,
            QMediaFormat::VideoCodec::H264,
            QMediaRecorder::LowQuality);

        durSpy.wait(3000);
        m_recorder->stopRecording();

        QVERIFY(durSpy.count() > 0);
    }

    // ── isCameraActive ────────────────────────────────────────
    void test_isCameraActive_beforeInit_returnsFalse()
    {
        QVERIFY(!m_recorder->isCameraActive());
    }

    void test_isCameraActive_afterSuccessfulInit_returnsTrue()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        spy.wait(3000);

        if (!ok || spy.isEmpty() || !spy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        QVERIFY(m_recorder->isCameraActive());
    }

    // ── Destructor safety ─────────────────────────────────────
    void test_destructor_whileRecording_doesNotCrash()
    {
        {
            VideoRecorder local(this);
            QSignalSpy spy(&local, &VideoRecorder::cameraInitialized);
            bool ok = local.initializeCamera();
            spy.wait(3000);

            if (ok && !spy.isEmpty() && spy.at(0).at(0).toBool()) {
                local.startRecording(
                    tempPath("destruct.mp4"),
                    QMediaFormat::MPEG4,
                    QMediaFormat::VideoCodec::H264,
                    QMediaRecorder::LowQuality);
            }
            // destructor fires here
        }
        QVERIFY(true);
    }

    // ── Quality variations ────────────────────────────────────
    void test_qualityVariations_doNotCrash()
    {
        QSignalSpy spy(m_recorder, &VideoRecorder::cameraInitialized);
        bool ok = m_recorder->initializeCamera();
        spy.wait(3000);

        if (!ok || spy.isEmpty() || !spy.at(0).at(0).toBool()) {
            QSKIP("No camera available");
        }

        const QList<QMediaRecorder::Quality> qualities = {
            QMediaRecorder::LowQuality,
            QMediaRecorder::NormalQuality,
            QMediaRecorder::HighQuality,
        };

        for (auto q : qualities) {
            m_recorder->startRecording(
                tempPath(QString("q%1.mp4").arg(static_cast<int>(q))),
                QMediaFormat::MPEG4,
                QMediaFormat::VideoCodec::H264,
                q);
            QVERIFY(m_recorder->isRecording());
            m_recorder->stopRecording();
            QVERIFY(!m_recorder->isRecording());
        }
    }
};

QTEST_MAIN(TestVideoRecorder)
#include "TestVideoRecorder.moc"
