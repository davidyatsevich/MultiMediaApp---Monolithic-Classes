#include <QtTest/QtTest>
#include "File.hpp"

class TestFile : public QObject
{
    Q_OBJECT

private slots:

    // ── Construction ─────────────────────────────────────────
    void test_construct_storesAllFields()
    {
        File f("/tmp/rec.m4a", "M4A", "rec.m4a");
        QCOMPARE(f.getFilePath(),   std::string("/tmp/rec.m4a"));
        QCOMPARE(f.getFileFormat(), std::string("M4A"));
        QCOMPARE(f.getFileName(),   std::string("rec.m4a"));
    }

    void test_construct_emptyStrings_allowed()
    {
        File f("", "", "");
        QCOMPARE(f.getFilePath(),   std::string(""));
        QCOMPARE(f.getFileFormat(), std::string(""));
        QCOMPARE(f.getFileName(),   std::string(""));
    }

    void test_construct_unicodeValues_preserved()
    {
        File f("/tmp/ünïcödé.m4a", "M4A", "ünïcödé.m4a");
        QCOMPARE(f.getFilePath(), std::string("/tmp/ünïcödé.m4a"));
        QCOMPARE(f.getFileName(), std::string("ünïcödé.m4a"));
    }

    void test_construct_longPath_preserved()
    {
        std::string longPath(512, 'a');
        longPath += ".m4a";
        File f(longPath, "M4A", "long.m4a");
        QCOMPARE(f.getFilePath(), longPath);
    }

    // ── getFilePath ───────────────────────────────────────────
    void test_getFilePath_returnsCorrectValue()
    {
        File f("/recordings/session1.wav", "WAV", "session1.wav");
        QCOMPARE(f.getFilePath(), std::string("/recordings/session1.wav"));
    }

    void test_getFilePath_doesNotReturnFormat()
    {
        File f("/tmp/a.mp4", "MP4", "a.mp4");
        QVERIFY(f.getFilePath() != f.getFileFormat());
    }

    // ── getFileFormat ─────────────────────────────────────────
    void test_getFileFormat_returnsCorrectValue()
    {
        File f("/tmp/a.flac", "FLAC", "a.flac");
        QCOMPARE(f.getFileFormat(), std::string("FLAC"));
    }

    void test_getFileFormat_isCaseSensitive()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        QVERIFY(f.getFileFormat() != std::string("m4a"));
        QCOMPARE(f.getFileFormat(), std::string("M4A"));
    }

    // ── getFileName ───────────────────────────────────────────
    void test_getFileName_returnsCorrectValue()
    {
        File f("/tmp/my_recording.ogg", "OGG", "my_recording.ogg");
        QCOMPARE(f.getFileName(), std::string("my_recording.ogg"));
    }

    void test_getFileName_independentOfPath()
    {
        File f("/very/deep/path/to/file.mp3", "MP3", "file.mp3");
        QCOMPARE(f.getFileName(), std::string("file.mp3"));
        QVERIFY(f.getFileName() != f.getFilePath());
    }

    // ── setFilePath ───────────────────────────────────────────
    void test_setFilePath_updatesValue()
    {
        File f("/tmp/old.m4a", "M4A", "old.m4a");
        f.setFilePath("/tmp/new.m4a");
        QCOMPARE(f.getFilePath(), std::string("/tmp/new.m4a"));
    }

    void test_setFilePath_doesNotAffectOtherFields()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFilePath("/tmp/b.m4a");
        QCOMPARE(f.getFileFormat(), std::string("M4A"));
        QCOMPARE(f.getFileName(),   std::string("a.m4a"));
    }

    void test_setFilePath_toEmpty_allowed()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFilePath("");
        QCOMPARE(f.getFilePath(), std::string(""));
    }

    void test_setFilePath_multipleUpdates()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFilePath("/tmp/b.m4a");
        f.setFilePath("/tmp/c.m4a");
        QCOMPARE(f.getFilePath(), std::string("/tmp/c.m4a"));
    }

    // ── setFileFormat ─────────────────────────────────────────
    void test_setFileFormat_updatesValue()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFileFormat("MP4");
        QCOMPARE(f.getFileFormat(), std::string("MP4"));
    }

    void test_setFileFormat_doesNotAffectOtherFields()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFileFormat("WAV");
        QCOMPARE(f.getFilePath(),   std::string("/tmp/a.m4a"));
        QCOMPARE(f.getFileName(),   std::string("a.m4a"));
    }

    // ── setFileName ───────────────────────────────────────────
    void test_setFileName_updatesValue()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFileName("renamed.m4a");
        QCOMPARE(f.getFileName(), std::string("renamed.m4a"));
    }

    void test_setFileName_doesNotAffectOtherFields()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFileName("renamed.m4a");
        QCOMPARE(f.getFilePath(),   std::string("/tmp/a.m4a"));
        QCOMPARE(f.getFileFormat(), std::string("M4A"));
    }

    // ── Combined setters ──────────────────────────────────────
    void test_allSetters_canUpdateAllFields()
    {
        File f("/tmp/a.m4a", "M4A", "a.m4a");
        f.setFilePath("/tmp/b.mp4");
        f.setFileFormat("MP4");
        f.setFileName("b.mp4");

        QCOMPARE(f.getFilePath(),   std::string("/tmp/b.mp4"));
        QCOMPARE(f.getFileFormat(), std::string("MP4"));
        QCOMPARE(f.getFileName(),   std::string("b.mp4"));
    }

    // ── Value semantics ───────────────────────────────────────
    void test_twoInstances_areIndependent()
    {
        File f1("/tmp/a.m4a", "M4A", "a.m4a");
        File f2("/tmp/b.mp4", "MP4", "b.mp4");

        f1.setFilePath("/tmp/changed.wav");
        // f2 must be untouched
        QCOMPARE(f2.getFilePath(), std::string("/tmp/b.mp4"));
    }

    // ── Supported format strings (as used in the app) ─────────
    void test_audioFormats_roundtrip()
    {
        const std::vector<std::string> formats = { "M4A","MP3","OGG","FLAC","WAV" };
        for (const auto& fmt : formats) {
            File f("/tmp/x", fmt, "x");
            QCOMPARE(f.getFileFormat(), fmt);
        }
    }

    void test_videoFormats_roundtrip()
    {
        const std::vector<std::string> formats = { "MP4","MOV","AVI","MPG","MPEG" };
        for (const auto& fmt : formats) {
            File f("/tmp/x", fmt, "x");
            QCOMPARE(f.getFileFormat(), fmt);
        }
    }
};

QTEST_MAIN(TestFile)
#include "TestFile.moc"
