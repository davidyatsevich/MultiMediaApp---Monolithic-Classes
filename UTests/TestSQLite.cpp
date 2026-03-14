#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QDir>
#include "SQLite.hpp"

class TestSQLite : public QObject
{
    Q_OBJECT

private:
    QString m_dbPath;
    SQLite *m_db;

private slots:

    // ── Fixture ──────────────────────────────────────────────
    void init()
    {
        // Use a unique temp file for each test so tests are isolated
        m_dbPath = QDir::tempPath() + "/test_multimedia_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".db";
        m_db = new SQLite(m_dbPath.toStdString());
    }

    void cleanup()
    {
        delete m_db;
        m_db = nullptr;
        QFile::remove(m_dbPath);
    }

    // ── Construction ─────────────────────────────────────────
    void test_construct_createsDatabase()
    {
        QVERIFY(QFile::exists(m_dbPath));
    }

    void test_construct_invalidPath_throws()
    {
        QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                                 SQLite bad("/nonexistent/path/that/cannot/exist.db"));
    }

    void test_construct_emptyDatabase_countIsZero()
    {
        QCOMPARE(m_db->getRecordingCount(), 0);
    }

    // ── insertRecording ───────────────────────────────────────
    void test_insert_singleRecord_countIsOne()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        QCOMPARE(m_db->getRecordingCount(), 1);
    }

    void test_insert_multipleRecords_countMatches()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.mp4", "MP4", "b.mp4");
        m_db->insertRecording("/tmp/c.wav", "WAV", "c.wav");
        QCOMPARE(m_db->getRecordingCount(), 3);
    }

    void test_insert_fieldsRoundtrip()
    {
        m_db->insertRecording("/tmp/test.m4a", "M4A", "test.m4a");
        auto recs = m_db->getAllRecordings();
        QCOMPARE(recs.size(), 1u);
        QCOMPARE(std::get<1>(recs[0]), std::string("/tmp/test.m4a"));
        QCOMPARE(std::get<2>(recs[0]), std::string("M4A"));
        QCOMPARE(std::get<3>(recs[0]), std::string("test.m4a"));
    }

    void test_insert_idIsPositive()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        QVERIFY(std::get<0>(recs[0]) > 0);
    }

    void test_insert_idsAreUnique()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");
        auto recs = m_db->getAllRecordings();
        QVERIFY(std::get<0>(recs[0]) != std::get<0>(recs[1]));
    }

    void test_insert_emptyStrings_doesNotThrow()
    {
        // Empty strings are valid SQL — should not throw
        QVERIFY_THROWS_NO_EXCEPTION(
            m_db->insertRecording("", "", ""));
        QCOMPARE(m_db->getRecordingCount(), 1);
    }

    void test_insert_unicodePath_roundtrips()
    {
        std::string path = "/tmp/ünïcödé_文件.m4a";
        m_db->insertRecording(path, "M4A", "ünïcödé_文件.m4a");
        auto recs = m_db->getAllRecordings();
        QCOMPARE(std::get<1>(recs[0]), path);
    }

    // ── getAllRecordings ──────────────────────────────────────
    void test_getAll_empty_returnsEmptyVector()
    {
        auto recs = m_db->getAllRecordings();
        QVERIFY(recs.empty());
    }

    void test_getAll_orderedByCreatedAtDesc()
    {
        // Insert with small delays so timestamps differ
        m_db->insertRecording("/tmp/first.m4a", "M4A", "first.m4a");
        QThread::msleep(10);
        m_db->insertRecording("/tmp/second.m4a", "M4A", "second.m4a");

        auto recs = m_db->getAllRecordings();
        // Most recent should come first (ORDER BY created_at DESC)
        QCOMPARE(std::get<3>(recs[0]), std::string("second.m4a"));
        QCOMPARE(std::get<3>(recs[1]), std::string("first.m4a"));
    }

    // ── getRecording (by id) ──────────────────────────────────
    void test_getRecording_validId_returnsCorrectData()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        auto rec = m_db->getRecording(id);
        QCOMPARE(std::get<1>(rec), std::string("/tmp/a.m4a"));
        QCOMPARE(std::get<2>(rec), std::string("M4A"));
        QCOMPARE(std::get<3>(rec), std::string("a.m4a"));
    }

    void test_getRecording_invalidId_throws()
    {
        QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                                 m_db->getRecording(99999));
    }

    // ── recordingExists ───────────────────────────────────────
    void test_exists_afterInsert_returnsTrue()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);
        QVERIFY(m_db->recordingExists(id));
    }

    void test_exists_nonExistentId_returnsFalse()
    {
        QVERIFY(!m_db->recordingExists(99999));
    }

    void test_exists_afterDelete_returnsFalse()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);
        m_db->deleteRecording(id);
        QVERIFY(!m_db->recordingExists(id));
    }

    // ── deleteRecording ───────────────────────────────────────
    void test_delete_existingRecord_countDecreases()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        m_db->deleteRecording(id);
        QCOMPARE(m_db->getRecordingCount(), 1);
    }

    void test_delete_existingRecord_removedFromGetAll()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        m_db->deleteRecording(id);
        QVERIFY(m_db->getAllRecordings().empty());
    }

    void test_delete_nonExistentId_doesNotThrow()
    {
        // Should warn but not throw
        QVERIFY_THROWS_NO_EXCEPTION(m_db->deleteRecording(99999));
    }

    void test_delete_doesNotAffectOtherRecords()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");
        auto recs = m_db->getAllRecordings();

        // Delete second (index 0 = most recent)
        int idToDelete = std::get<0>(recs[0]);
        m_db->deleteRecording(idToDelete);

        auto remaining = m_db->getAllRecordings();
        QCOMPARE(remaining.size(), 1u);
        QVERIFY(std::get<0>(remaining[0]) != idToDelete);
    }

    // ── editRecording ─────────────────────────────────────────
    void test_edit_updatesAllFields()
    {
        m_db->insertRecording("/tmp/old.m4a", "M4A", "old.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        m_db->editRecording(id, "/tmp/new.mp4", "MP4", "new.mp4");
        auto rec = m_db->getRecording(id);

        QCOMPARE(std::get<1>(rec), std::string("/tmp/new.mp4"));
        QCOMPARE(std::get<2>(rec), std::string("MP4"));
        QCOMPARE(std::get<3>(rec), std::string("new.mp4"));
    }

    void test_edit_nonExistentId_doesNotThrow()
    {
        QVERIFY_THROWS_NO_EXCEPTION(
            m_db->editRecording(99999, "/tmp/x.m4a", "M4A", "x.m4a"));
    }

    void test_edit_doesNotChangeCount()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        auto recs = m_db->getAllRecordings();
        int id = std::get<0>(recs[0]);

        m_db->editRecording(id, "/tmp/b.mp4", "MP4", "b.mp4");
        QCOMPARE(m_db->getRecordingCount(), 1);
    }

    void test_edit_doesNotAffectOtherRecords()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");
        auto recs = m_db->getAllRecordings();

        int idToEdit = std::get<0>(recs[0]); // most recent
        int idOther = std::get<0>(recs[1]);

        m_db->editRecording(idToEdit, "/tmp/changed.mp4", "MP4", "changed.mp4");

        auto other = m_db->getRecording(idOther);
        QCOMPARE(std::get<1>(other), std::string("/tmp/b.m4a"));
    }

    // ── getRecordingCount ─────────────────────────────────────
    void test_count_afterMixedOperations()
    {
        m_db->insertRecording("/tmp/a.m4a", "M4A", "a.m4a");
        m_db->insertRecording("/tmp/b.m4a", "M4A", "b.m4a");
        m_db->insertRecording("/tmp/c.m4a", "M4A", "c.m4a");

        auto recs = m_db->getAllRecordings();
        m_db->deleteRecording(std::get<0>(recs[0]));

        m_db->insertRecording("/tmp/d.m4a", "M4A", "d.m4a");

        QCOMPARE(m_db->getRecordingCount(), 3);
    }

    // ── Persistence across instances ──────────────────────────
    void test_persist_dataAvailableAfterReopeningDb()
    {
        m_db->insertRecording("/tmp/persist.m4a", "M4A", "persist.m4a");
        delete m_db;

        // Re-open the same file
        m_db = new SQLite(m_dbPath.toStdString());
        QCOMPARE(m_db->getRecordingCount(), 1);

        auto recs = m_db->getAllRecordings();
        QCOMPARE(std::get<3>(recs[0]), std::string("persist.m4a"));
    }
};

QTEST_MAIN(TestSQLite)
#include "TestSQLite.moc"