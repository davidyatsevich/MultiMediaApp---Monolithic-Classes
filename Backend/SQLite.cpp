#include "SQLite.hpp"
#include <QDebug>

SQLite::SQLite(const std::string &dbName)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QString::fromStdString(dbName));
    if (!db.open())
    {
        throw std::runtime_error("Failed to open database: " + db.lastError().text().toStdString());
    }

    QSqlQuery query;
    QString createTable = "CREATE TABLE IF NOT EXISTS recordings ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "filepath TEXT NOT NULL, "
                          "format TEXT NOT NULL, "
                          "filename TEXT NOT NULL, "
                          "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                          ")";

    if (!query.exec(createTable))
    {
        qWarning() << "Failed to create table:" << query.lastError().text();
        throw std::runtime_error("Failed to create table: " + query.lastError().text().toStdString());
    }
}

SQLite::~SQLite()
{
    if (db.isOpen())
    {
        db.close();
    }
}

void SQLite::insertRecording(const std::string &filePath, const std::string &format, const std::string &fileName)
{
    QSqlQuery query;
    query.prepare("INSERT INTO recordings (filepath, format, filename) VALUES (:filepath, :format, :filename)");
    query.bindValue(":filepath", QString::fromStdString(filePath));
    query.bindValue(":format", QString::fromStdString(format));
    query.bindValue(":filename", QString::fromStdString(fileName));

    if (!query.exec())
    {
        qWarning() << "Failed to insert recording:" << query.lastError().text();
        throw std::runtime_error("Failed to insert recording: " + query.lastError().text().toStdString());
    }
}

void SQLite::editRecording(int id, const std::string &newFilePath, const std::string &newFormat, const std::string &newFileName)
{
    QSqlQuery query;
    query.prepare("UPDATE recordings SET filepath = :filepath, format = :format, filename = :filename WHERE id = :id");
    query.bindValue(":filepath", QString::fromStdString(newFilePath));
    query.bindValue(":format", QString::fromStdString(newFormat));
    query.bindValue(":filename", QString::fromStdString(newFileName));
    query.bindValue(":id", id);

    if (!query.exec())
    {
        qWarning() << "Failed to edit recording:" << query.lastError().text();
        throw std::runtime_error("Failed to edit recording: " + query.lastError().text().toStdString());
    }

    // Check if any rows were actually updated
    if (query.numRowsAffected() == 0)
    {
        qWarning() << "No recording found with id:" << id;
    }
}

void SQLite::deleteRecording(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM recordings WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec())
    {
        qWarning() << "Failed to delete recording:" << query.lastError().text();
        throw std::runtime_error("Failed to delete recording: " + query.lastError().text().toStdString());
    }

    // Check if any rows were actually deleted
    if (query.numRowsAffected() == 0)
    {
        qWarning() << "No recording found with id:" << id;
    }
}

std::vector<std::tuple<int, std::string, std::string, std::string>> SQLite::getAllRecordings()
{
    std::vector<std::tuple<int, std::string, std::string, std::string>> recordings;
    QSqlQuery query("SELECT id, filepath, format, filename FROM recordings ORDER BY created_at DESC");

    if (!query.exec())
    {
        qWarning() << "Failed to retrieve recordings:" << query.lastError().text();
        return recordings; // Return empty vector on error
    }

    while (query.next())
    {
        int id = query.value(0).toInt();
        std::string filePath = query.value(1).toString().toStdString();
        std::string format = query.value(2).toString().toStdString();
        std::string fileName = query.value(3).toString().toStdString();
        recordings.emplace_back(id, filePath, format, fileName);
    }

    return recordings;
}

// Optional: Add additional helper methods
bool SQLite::recordingExists(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM recordings WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next())
    {
        return query.value(0).toInt() > 0;
    }
    return false;
}

std::tuple<int, std::string, std::string, std::string> SQLite::getRecording(int id)
{
    QSqlQuery query;
    query.prepare("SELECT id, filepath, format, filename FROM recordings WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next())
    {
        int recordId = query.value(0).toInt();
        std::string filePath = query.value(1).toString().toStdString();
        std::string format = query.value(2).toString().toStdString();
        std::string fileName = query.value(3).toString().toStdString();
        return std::make_tuple(recordId, filePath, format, fileName);
    }

    throw std::runtime_error("Recording not found with id: " + std::to_string(id));
}

int SQLite::getRecordingCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM recordings");
    if (query.exec() && query.next())
    {
        return query.value(0).toInt();
    }
    return 0;
}
