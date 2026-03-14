#include <QtTest/QtTest>
#include <QDir>
#include "SQLite.hpp"
#include "File.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Integration tests: SQLite + File working together as the app does.
// Exercises the full round-trip: insert via DB, reconstruct File objects,
// verify getters, edit, delete.
// ─────────────────────────────────────────────────────────────────────────────

class TestSQLiteFileIntegration : public QObject
{
    Q_OBJECT

private:
    QString  m_dbPath;
    SQLite*  m_db;

    // Reconstruct File objects from DB exactly as the app does
    // (see PlaybackTab::loadRecordings, AudioEditTab::loadRecordings, etc.)
    std::vector<std::shared_ptr<File>> loadFilesFromDb()
    {
        std::vector<std::shared_ptr<File>> files;
        for (const auto& rec : m_db->getAllRecordings()) {
            files.push_back(std::make_shared<File>(
                std::get<1>(rec),   // filePath
                std::get<2>(rec),   // format
                std::get<3>(rec)    // fileName
            ));
        }
        return files;
    }

private slots:

    void init()
    {
        m_dbPath = QDir::tempPath() + "/integration_"
                 + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".db";
        m_db = new SQLite(m_dbPath.toStdString());
    }

    void cleanup()
    {
        delete m_db;
        m_db = nullptr;
        QFile::remove(m_dbPath);
    }

    // ── Round-trip: insert → load as File objects ─────────────
    void test_insertAndLoad_singleAudioFile()
    {
        m_db->insertRecording("/tmp/rec.m4a", "M4A", "rec.m4a");
        auto files = loadFilesFromDb();

        QCOMPARE(files.size(), 1u);
        QCOMPARE(files[0]->getFilePath(),   std::string("/tmp/rec.m4a"));
        QCOMPARE(files[0]->getFileFormat(), std::string("M4A"));
        QCOMPARE(files[0]->getFileName(),   std::string("rec.m4a"));
    }

    void test_insertAndLoad_singleVideoFile()
    {
        m_db->insertRecording("/tmp/video.mp4", "MP4", "video.mp4");
        auto files = loadFilesFromDb();

        QCOMPARE(files.size(), 1u);
        QCOMPARE(files[0]->getFileFormat(), std::string("MP4"));
    }

    void test_insertAndLoad_mixedFormats()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.mp4", "MP4", "b.mp4");
        m_db->insertRecording("/tmp/c.wav", "WAV", "c.wav");

        auto files = loadFilesFromDb();
        QCOMPARE(files.size(), 3u);
    }

    // ── Filter audio vs video (as AudioEditTab / VideoEditTab do) ──
    void test_filterAudioFiles_onlyAudioFormatsReturned()
    {
        m_db->insertRecording("/tmp/a.m4a",  "M4A",  "a.m4a");
        m_db->insertRecording("/tmp/b.mp4",  "MP4",  "b.mp4");
        m_db->insertRecording("/tmp/c.ogg",  "OGG",  "c.ogg");
        m_db->insertRecording("/tmp/d.flac", "FLAC", "d.flac");
        m_db->insertRecording("/tmp/e.wav",  "WAV",  "e.wav");
        m_db->insertRecording("/tmp/f.mov",  "MOV",  "f.mov");

        auto files = loadFilesFromDb();

        // Replicate the filter from AudioEditTab::updateSelectors()
        const QSet<QString> audioFmts = { "m4a","mp3","ogg","flac","wav" };
        int audioCount = 0;
        for (const auto& f : files) {
            QString fmt = QString::fromStdString(f->getFileFormat()).toLower();
            if (audioFmts.contains(fmt)) audioCount++;
        }
        QCOMPARE(audioCount, 4); // m4a, ogg, flac, wav
    }

    void test_filterVideoFiles_onlyVideoFormatsReturned()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.mp4", "MP4", "b.mp4");
        m_db->insertRecording("/tmp/c.mov", "MOV", "c.mov");
        m_db->insertRecording("/tmp/d.avi", "AVI", "d.avi");
        m_db->insertRecording("/tmp/e.mpg", "MPG", "e.mpg");

        auto files = loadFilesFromDb();

        const QSet<QString> videoFmts = { "mp4","mov","avi","mpg","mpeg" };
        int videoCount = 0;
        for (const auto& f : files) {
            QString fmt = QString::fromStdString(f->getFileFormat()).toLower();
            if (videoFmts.contains(fmt)) videoCount++;
        }
        QCOMPARE(videoCount, 4); // mp4, mov, avi, mpg
    }

    // ── Edit via DB → reload as File ──────────────────────────
    void test_editViaDb_reflectedInFileObject()
    {
        m_db->insertRecording("/tmp/old.m4a", "M4A", "old.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        m_db->editRecording(id, "/tmp/new.mp4", "MP4", "new.mp4");

        auto files = loadFilesFromDb();
        QCOMPARE(files[0]->getFilePath(),   std::string("/tmp/new.mp4"));
        QCOMPARE(files[0]->getFileFormat(), std::string("MP4"));
        QCOMPARE(files[0]->getFileName(),   std::string("new.mp4"));
    }

    // ── Delete via DB → File list shrinks ─────────────────────
    void test_deleteViaDb_removedFromFileList()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");

        auto recs = m_db->getAllRecordings();
        int idToDelete = std::get<0>(recs[0]); // most recent

        m_db->deleteRecording(idToDelete);

        auto files = loadFilesFromDb();
        QCOMPARE(files.size(), 1u);
        QCOMPARE(files[0]->getFileName(), std::string("a.m4a"));
    }

    // ── File setters do not affect DB ────────────────────────
    void test_mutatingFileObject_doesNotChangeDb()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto files = loadFilesFromDb();

        // Mutate the in-memory File object
        files[0]->setFilePath("/tmp/mutated.mp4");
        files[0]->setFileFormat("MP4");
        files[0]->setFileName("mutated.mp4");

        // DB must be unchanged
        auto dbRecs = m_db->getAllRecordings();
        QCOMPARE(std::get<1>(dbRecs[0]), std::string("/tmp/a.m4a"));
        QCOMPARE(std::get<2>(dbRecs[0]), std::string("M4A"));
        QCOMPARE(std::get<3>(dbRecs[0]), std::string("a.m4a"));
    }

    // ── ComboBox population simulation ───────────────────────
    // Reproduces what PlaybackTab::updateSelector() does:
    // build a name list and verify it matches the DB order
    void test_comboBoxPopulation_namesMatchDbOrder()
    {
        m_db->insertRecording("/tmp/first.m4a",  "M4A", "first.m4a");
        QThread::msleep(10);
        m_db->insertRecording("/tmp/second.m4a", "M4A", "second.m4a");

        auto files = loadFilesFromDb(); // ordered newest first

        QStringList names;
        for (const auto& f : files)
            names << QString::fromStdString(f->getFileName());

        QCOMPARE(names.at(0), QString("second.m4a")); // most recent first
        QCOMPARE(names.at(1), QString("first.m4a"));
    }

    // ── Shared pointer ownership semantics ───────────────────
    void test_sharedPtr_fileLifetimeIndependentOfVector()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");

        std::shared_ptr<File> kept;
        {
            auto files = loadFilesFromDb();
            kept = files[0]; // grab a reference
        } // files vector destroyed here

        // kept should still be valid
        QCOMPARE(kept->getFileName(), std::string("a.m4a"));
    }

    // ── Cleanup storage simulation ────────────────────────────
    // Reproduces StorageTab::cleanupStorage(): build a QSet of
    // DB paths, check which files are orphaned
    void test_cleanupLogic_identifiesOrphanedPaths()
    {
        m_db->insertRecording("/tmp/exists.m4a",   "M4A", "exists.m4a");
        m_db->insertRecording("/tmp/also_exists.m4a", "M4A", "also_exists.m4a");

        auto dbRecs = m_db->getAllRecordings();
        QSet<QString> dbPaths;
        for (const auto& rec : dbRecs)
            dbPaths.insert(QString::fromStdString(std::get<1>(rec)));

        // Simulate files on disk
        QStringList onDisk = {
            "/tmp/exists.m4a",
            "/tmp/also_exists.m4a",
            "/tmp/orphan1.m4a",   // not in DB
            "/tmp/orphan2.m4a",   // not in DB
        };

        QStringList orphans;
        for (const QString& path : onDisk) {
            if (!dbPaths.contains(path))
                orphans << path;
        }

        QCOMPARE(orphans.size(), 2);
        QVERIFY(orphans.contains("/tmp/orphan1.m4a"));
        QVERIFY(orphans.contains("/tmp/orphan2.m4a"));
    }
};

QTEST_MAIN(TestSQLiteFileIntegration)
#include "TestSQLiteFileIntegration.moc"
