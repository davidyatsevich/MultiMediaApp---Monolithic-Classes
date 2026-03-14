#pragma once
#include <string>

class File
{
private:
    std::string filePath;
    std::string fileFormat;
    std::string fileName;

public:
    File(std::string fP, std::string fF, std::string fN) : filePath(fP), fileFormat(fF), fileName(fN) {}
    ~File() = default;
    std::string getFilePath() const
    {
        return filePath;
    }
    std::string getFileFormat() const
    {
        return fileFormat;
    }
    std::string getFileName() const
    {
        return fileName;
    }
    void setFilePath(const std::string &fP)
    {
        filePath = fP;
    }
    void setFileFormat(const std::string &fF)
    {
        fileFormat = fF;
    }
    void setFileName(const std::string &fN)
    {
        fileName = fN;
    }
};
