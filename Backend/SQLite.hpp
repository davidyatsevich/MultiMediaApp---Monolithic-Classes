#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <string>
#include <vector>
#include <tuple>

class SQLite
{
private:
    QSqlDatabase db;

public:
    SQLite(const std::string &dbName);
    ~SQLite();

    // Core CRUD operations
    void insertRecording(const std::string &filePath, const std::string &format, const std::string &fileName);
    void editRecording(int id, const std::string &newFilePath, const std::string &newFormat, const std::string &newFileName);
    void deleteRecording(int id);
    std::vector<std::tuple<int, std::string, std::string, std::string>> getAllRecordings();

    // Optional: Additional helper methods
    bool recordingExists(int id);
    std::tuple<int, std::string, std::string, std::string> getRecording(int id);
    int getRecordingCount();
};
