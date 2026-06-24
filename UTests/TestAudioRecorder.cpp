#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioInput>
#include "Recorders.hpp"

class TestAudioRecorder : public QObject
{
    Q_OBJECT

private:
    AudioRecorder *m_recorder;
    QTemporaryDir m_tempDir;

    QString tempPath(const QString &name) const
    {
        return m_tempDir.filePath(name);
    }

private slots:

    // ── Fixture ──────────────────────────────────────────────
    void init()
    {
        m_recorder = new AudioRecorder(this);
    }
    bool hasAudioInput() const
    {
        return !QMediaDevices::audioInputs().isEmpty();
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

    void test_initialState_errorStringEmpty()
    {
        QVERIFY(m_recorder->errorString().isEmpty());
    }

    // ── startRecording ────────────────────────────────────────
    void test_startRecording_setsIsRecordingTrue()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        m_recorder->startRecording(
            tempPath("test.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    void test_startRecording_whenAlreadyRecording_isNoOp()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        m_recorder->startRecording(
            tempPath("test1.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        // Second call should be silently ignored
        m_recorder->startRecording(
            tempPath("test2.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    // ── stopRecording ─────────────────────────────────────────
    void test_stopRecording_setsIsRecordingFalse()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        m_recorder->startRecording(
            tempPath("test.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        m_recorder->stopRecording();
        QVERIFY(!m_recorder->isRecording());
    }

    void test_stopRecording_whenNotRecording_isNoOp()
    {
        // Should not crash or throw
        QVERIFY_THROWS_NO_EXCEPTION(m_recorder->stopRecording());
        QVERIFY(!m_recorder->isRecording());
    }

    void test_stopRecording_emitsRecordingStoppedSignal()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        QSignalSpy spy(m_recorder, &AudioRecorder::recordingStopped);

        m_recorder->startRecording(
            tempPath("test.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);
        m_recorder->stopRecording();

        QCOMPARE(spy.count(), 1);
    }

    void test_stopRecording_whenNotRecording_doesNotEmitSignal()
    {
        QSignalSpy spy(m_recorder, &AudioRecorder::recordingStopped);
        m_recorder->stopRecording();
        QCOMPARE(spy.count(), 0);
    }

    // ── recordingStopped signal ───────────────────────────────
    void test_recordingStopped_emittedExactlyOnce_perSession()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        QSignalSpy spy(m_recorder, &AudioRecorder::recordingStopped);

        m_recorder->startRecording(
            tempPath("a.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);
        m_recorder->stopRecording();

        m_recorder->startRecording(
            tempPath("b.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);
        m_recorder->stopRecording();

        // Two sessions → two signals
        QCOMPARE(spy.count(), 2);
    }

    // ── durationChanged signal ────────────────────────────────
    void test_durationChanged_emittedDuringRecording()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        QSignalSpy spy(m_recorder, &AudioRecorder::durationChanged);

        m_recorder->startRecording(
            tempPath("dur.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        // Wait up to 3 seconds for at least one duration update
        spy.wait(3000);
        m_recorder->stopRecording();

        QVERIFY(spy.count() > 0);
    }

    void test_durationChanged_valueIsNonNegative()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        QSignalSpy spy(m_recorder, &AudioRecorder::durationChanged);

        m_recorder->startRecording(
            tempPath("dur2.m4a"),
            QMediaFormat::MPEG4,
            QMediaFormat::AudioCodec::AAC);

        spy.wait(2000);
        m_recorder->stopRecording();

        for (const QList<QVariant> &args : spy)
        {
            qint64 duration = args.at(0).toLongLong();
            QVERIFY(duration >= 0);
        }
    }

    // ── Format variations ─────────────────────────────────────
    void test_startRecording_mp3Format_doesNotCrash()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        m_recorder->startRecording(
            tempPath("test.mp3"),
            QMediaFormat::MP3,
            QMediaFormat::AudioCodec::MP3);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    void test_startRecording_wavFormat_doesNotCrash()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        m_recorder->startRecording(
            tempPath("test.wav"),
            QMediaFormat::Wave,
            QMediaFormat::AudioCodec::Wave);

        QVERIFY(m_recorder->isRecording());
        m_recorder->stopRecording();
    }

    // ── Destructor safety ─────────────────────────────────────
    void test_destructor_whileRecording_doesNotCrash()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        // Create a recorder, start it, then let it go out of scope
        // The destructor should call stopRecording() safely
        {
            AudioRecorder local(this);
            local.startRecording(
                tempPath("destruct.m4a"),
                QMediaFormat::MPEG4,
                QMediaFormat::AudioCodec::AAC);
            // destructor called here — should not crash
        }
        QVERIFY(true); // reached here means no crash
    }

    // ── Start / stop cycles ───────────────────────────────────
    void test_multipleStartStop_cycles_stateIsConsistent()
    {
        if (!hasAudioInput())
            QSKIP("No microphone available on this machine — skipping");

        for (int i = 0; i < 3; ++i)
        {
            m_recorder->startRecording(
                tempPath(QString("cycle%1.m4a").arg(i)),
                QMediaFormat::MPEG4,
                QMediaFormat::AudioCodec::AAC);
            QVERIFY(m_recorder->isRecording());

            m_recorder->stopRecording();
            QVERIFY(!m_recorder->isRecording());
        }
    }
};

QTEST_MAIN(TestAudioRecorder)
#include "TestAudioRecorder.moc"
