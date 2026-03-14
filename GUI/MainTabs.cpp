#include "MainTabs.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QFileInfo>

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QProcess>

MainTabs::MainTabs(QWidget *parent, std::shared_ptr<SQLite> database,
                   const QString &storageDirectory)
    : QTabWidget(parent), m_database(database), m_storageDirectory(storageDirectory)
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    setupUI();
    loadRecordings();
    updateSelectors();
}

MainTabs::~MainTabs()
{
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void MainTabs::setupUI()
{
    // --- Recording tab ---
    m_recordingTab = new QWidget;
    QVBoxLayout *recordingLayout = new QVBoxLayout(m_recordingTab);

    QTabWidget *recordingInnerTabs = new QTabWidget;
    recordingLayout->addWidget(recordingInnerTabs);

    m_audioRecordingTab = new NestedRecordingTabs(this, m_database, m_storageDirectory, NestedRecordingTabs::Mode::Audio);
    m_videoRecordingTab = new NestedRecordingTabs(this, m_database, m_storageDirectory, NestedRecordingTabs::Mode::Video);

    recordingInnerTabs->addTab(m_audioRecordingTab, "Audio Recording");
    recordingInnerTabs->addTab(m_videoRecordingTab, "Video Recording");

    connect(m_audioRecordingTab, &NestedRecordingTabs::recordingSaved,
            this, &MainTabs::recordingSaved);
    connect(m_videoRecordingTab, &NestedRecordingTabs::recordingSaved,
            this, &MainTabs::recordingSaved);

    addTab(m_recordingTab, "Recording");

    // --- Playback tab ---
    m_playbackTab = new QWidget;
    QVBoxLayout *playbackLayout = new QVBoxLayout(m_playbackTab);

    QGroupBox *selectorGroup = new QGroupBox("Select Recording");
    QVBoxLayout *selectorLayout = new QVBoxLayout(selectorGroup);
    m_recordingSelector = new QComboBox;
    selectorLayout->addWidget(m_recordingSelector);
    playbackLayout->addWidget(selectorGroup);

    QGroupBox *statusGroup = new QGroupBox("Playback Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    m_statusLabel = new QLabel("No recording selected");
    m_statusLabel->setStyleSheet("font-size: 12pt;");
    statusLayout->addWidget(m_statusLabel);
    m_timelineSlider = new QSlider(Qt::Horizontal);
    m_timelineSlider->setEnabled(false);
    statusLayout->addWidget(m_timelineSlider);
    playbackLayout->addWidget(statusGroup);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    m_playButton = new QPushButton("Play");
    m_playButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-size: 14pt; padding: 10px; }");
    m_playButton->setEnabled(false);
    connect(m_playButton, &QPushButton::clicked, this, &MainTabs::playRecording);
    controlLayout->addWidget(m_playButton);

    m_pauseButton = new QPushButton("Pause");
    m_pauseButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-size: 14pt; padding: 10px; }");
    m_pauseButton->setEnabled(false);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainTabs::pauseRecording);
    controlLayout->addWidget(m_pauseButton);

    playbackLayout->addLayout(controlLayout);
    playbackLayout->addStretch();

    addTab(m_playbackTab, "Playback");

    // --- Edit tab ---
    m_editTab = new QWidget;
    QVBoxLayout *editLayout = new QVBoxLayout(m_editTab);

    QTabWidget *editInnerTabs = new QTabWidget;
    editLayout->addWidget(editInnerTabs);

    m_audioEditTab = new NestedEditingTabs(this, m_database, NestedEditingTabs::Mode::Audio);
    m_videoEditTab = new NestedEditingTabs(this, m_database, NestedEditingTabs::Mode::Video);

    editInnerTabs->addTab(m_audioEditTab, "Audio Edit");
    editInnerTabs->addTab(m_videoEditTab, "Video Edit");

    addTab(m_editTab, "Edit");

    // --- Storage tab ---
    m_storageTab = new QWidget;
    QVBoxLayout *storageLayout = new QVBoxLayout(m_storageTab);

    m_titleLabel = new QLabel("Saved Recordings");
    m_titleLabel->setStyleSheet("font-size: 16pt; font-weight: bold;");
    storageLayout->addWidget(m_titleLabel);

    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"ID", "File Name", "Format", "File Path"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    storageLayout->addWidget(m_table);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    m_addButton = new QPushButton("Add Recording");
    m_addButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    connect(m_addButton, &QPushButton::clicked, this, &MainTabs::addRecording);
    buttonLayout->addWidget(m_addButton);

    m_editButton = new QPushButton("Edit Recording");
    m_editButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
    connect(m_editButton, &QPushButton::clicked, this, &MainTabs::editRecording);
    buttonLayout->addWidget(m_editButton);

    m_deleteButton = new QPushButton("Delete Recording");
    m_deleteButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px; }");
    connect(m_deleteButton, &QPushButton::clicked, this, &MainTabs::deleteRecording);
    buttonLayout->addWidget(m_deleteButton);

    m_refreshButton = new QPushButton("Refresh");
    m_refreshButton->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; padding: 8px; }");
    connect(m_refreshButton, &QPushButton::clicked, this, &MainTabs::refreshTable);
    buttonLayout->addWidget(m_refreshButton);

    m_cleanupButton = new QPushButton("Cleanup Storage");
    m_cleanupButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 8px; }");
    m_cleanupButton->setToolTip("Remove orphaned files from storage");
    connect(m_cleanupButton, &QPushButton::clicked, this, &MainTabs::cleanupStorage);
    buttonLayout->addWidget(m_cleanupButton);

    m_openStorageButton = new QPushButton("Open Storage Folder");
    m_openStorageButton->setStyleSheet("QPushButton { background-color: #607D8B; color: white; padding: 8px; }");
    connect(m_openStorageButton, &QPushButton::clicked, this, &MainTabs::openStorageFolder);
    buttonLayout->addWidget(m_openStorageButton);

    m_importButton = new QPushButton("Import from Explorer");
    m_importButton->setStyleSheet("QPushButton { background-color: #673AB7; color: white; padding: 8px; }");
    m_importButton->setToolTip("Import multiple files from file explorer");
    connect(m_importButton, &QPushButton::clicked, this, &MainTabs::importFromExplorer);
    buttonLayout->addWidget(m_importButton);

    m_exportButton = new QPushButton("Export to Explorer");
    m_exportButton->setStyleSheet("QPushButton { background-color: #009688; color: white; padding: 8px; }");
    m_exportButton->setToolTip("Export selected recording to a location");
    connect(m_exportButton, &QPushButton::clicked, this, &MainTabs::exportToExplorer);
    buttonLayout->addWidget(m_exportButton);

    buttonLayout->addStretch();
    storageLayout->addLayout(buttonLayout);

    addTab(m_storageTab, "Storage");
}

void MainTabs::initializeCamera()
{
    m_videoRecordingTab->initializeCamera();
}

// ─── Playback ─────────────────────────────────────────────────────────────────

void MainTabs::loadRecordings()
{
    m_recordings.clear();
    auto dbRecordings = m_database->getAllRecordings();

    for (const auto &rec : dbRecordings)
    {
        auto file = std::make_shared<File>(
            std::get<1>(rec), // filepath
            std::get<2>(rec), // format
            std::get<3>(rec)  // filename
        );
        m_recordings.push_back(file);
    }
}

void MainTabs::updateSelectors()
{
    loadRecordings();
    m_recordingSelector->clear();

    for (const auto &rec : m_recordings)
    {
        m_recordingSelector->addItem(QString::fromStdString(rec->getFileName()));
    }

    m_playButton->setEnabled(!m_recordings.empty());

    m_audioEditTab->updateSelectors();
    m_videoEditTab->updateSelectors();
}

void MainTabs::playRecording()
{
    int index = m_recordingSelector->currentIndex();
    if (index >= 0 && index < static_cast<int>(m_recordings.size()))
    {
        QString filePath = QString::fromStdString(m_recordings[index]->getFilePath());
        QString fileName = QString::fromStdString(m_recordings[index]->getFileName());
        QString format = QString::fromStdString(m_recordings[index]->getFileFormat()).toLower();

        if (format == "mp4" || format == "mov" || format == "avi" ||
            format == "mpg" || format == "mpeg")
        {
            VideoPlayerWindow *videoWindow = new VideoPlayerWindow(filePath, fileName, this);
            videoWindow->setAttribute(Qt::WA_DeleteOnClose);
            videoWindow->show();

            m_statusLabel->setText("Playing video in popup: " + fileName);
        }
        else
        {
            m_player->setSource(QUrl::fromLocalFile(filePath));
            m_player->play();

            m_statusLabel->setText("Playing: " + fileName);
            m_pauseButton->setEnabled(true);
        }
    }
}

void MainTabs::pauseRecording()
{
    m_player->pause();
    m_statusLabel->setText("Paused");
}

// ─── Storage ──────────────────────────────────────────────────────────────────

void MainTabs::refreshTable()
{
    m_table->setRowCount(0);
    auto dbRecordings = m_database->getAllRecordings();

    for (size_t i = 0; i < dbRecordings.size(); ++i)
    {
        m_table->insertRow(i);
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(std::get<0>(dbRecordings[i]))));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(std::get<3>(dbRecordings[i]))));
        m_table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(std::get<2>(dbRecordings[i]))));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(std::get<1>(dbRecordings[i]))));
    }

    emit recordingsChanged();
}

void MainTabs::addRecording()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Audio/Video File",
        m_storageDirectory,
        "Media Files (*.m4a *.mp3 *.ogg *.flac *.wav *.mp4 *.mov *.avi *.mpg);;All Files (*)");

    if (!filePath.isEmpty())
    {
        QFileInfo fileInfo(filePath);

        auto reply = QMessageBox::question(
            this,
            "Copy File",
            "Do you want to copy this file to the application's storage directory?",
            QMessageBox::Yes | QMessageBox::No);

        QString finalPath = filePath;
        if (reply == QMessageBox::Yes)
        {
            QString newPath = QDir(m_storageDirectory).filePath(fileInfo.fileName());

            int counter = 1;
            while (QFile::exists(newPath))
            {
                QString baseName = fileInfo.baseName();
                QString suffix = fileInfo.suffix();
                newPath = QDir(m_storageDirectory).filePath(QString("%1_%2.%3").arg(baseName).arg(counter++).arg(suffix));
            }

            if (QFile::copy(filePath, newPath))
            {
                finalPath = newPath;
            }
            else
            {
                QMessageBox::warning(this, "Copy Failed", "Failed to copy file. Using original path.");
            }
        }

        try
        {
            m_database->insertRecording(
                finalPath.toStdString(),
                fileInfo.suffix().toUpper().toStdString(),
                fileInfo.fileName().toStdString());

            QMessageBox::information(this, "Success", "Recording added successfully!");
            refreshTable();
        }
        catch (const std::exception &e)
        {
            QMessageBox::warning(this, "Error", QString("Failed to add recording: %1").arg(e.what()));
        }
    }
}

void MainTabs::editRecording()
{
    int currentRow = m_table->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "No Selection", "Please select a recording to edit.");
        return;
    }

    QMessageBox::information(this, "Edit Feature",
                             "Edit functionality can be implemented based on requirements (e.g., rename, change format).");
}

void MainTabs::deleteRecording()
{
    int currentRow = m_table->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "No Selection", "Please select a recording to delete.");
        return;
    }

    int id = m_table->item(currentRow, 0)->text().toInt();
    QString filePath = m_table->item(currentRow, 3)->text();
    QString fileName = m_table->item(currentRow, 1)->text();

    QString message = QString("Are you sure you want to delete this recording?\n\n"
                              "File: %1\n"
                              "Location: %2\n\n"
                              "This will permanently delete both the database entry and the file from storage.")
                          .arg(fileName, filePath);

    auto reply = QMessageBox::question(
        this,
        "Confirm Delete",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        try
        {
            QFile file(filePath);
            bool fileDeleted = false;

            if (file.exists())
            {
                if (file.remove())
                {
                    fileDeleted = true;
                }
                else
                {
                    QMessageBox::warning(
                        this,
                        "File Delete Warning",
                        QString("Could not delete the file:\n%1\n\nThe database entry will still be removed.\n\nError: %2")
                            .arg(filePath, file.errorString()));
                }
            }
            else
            {
                QMessageBox::information(
                    this,
                    "File Not Found",
                    "The file no longer exists in storage. Removing database entry only.");
            }

            m_database->deleteRecording(id);

            if (fileDeleted)
            {
                QMessageBox::information(this, "Success", "Recording and file deleted successfully!");
            }
            else
            {
                QMessageBox::information(this, "Partial Success",
                                         "Database entry removed. File was already missing or could not be deleted.");
            }

            refreshTable();
        }
        catch (const std::exception &e)
        {
            QMessageBox::warning(this, "Error",
                                 QString("Failed to delete recording from database: %1").arg(e.what()));
        }
    }
}

void MainTabs::cleanupStorage()
{
    auto reply = QMessageBox::question(
        this,
        "Cleanup Storage",
        "This will scan the storage directory and remove files that are not in the database.\n\n"
        "Do you want to continue?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    try
    {
        auto dbRecordings = m_database->getAllRecordings();
        QSet<QString> dbFilePaths;
        for (const auto &rec : dbRecordings)
            dbFilePaths.insert(QString::fromStdString(std::get<1>(rec)));

        QDir storageDir(m_storageDirectory);
        if (!storageDir.exists())
        {
            QMessageBox::information(this, "Storage Empty", "Storage directory does not exist.");
            return;
        }

        QStringList filters;
        filters << "*.m4a" << "*.mp3" << "*.ogg" << "*.flac" << "*.wav"
                << "*.mp4" << "*.mov" << "*.avi" << "*.mpg" << "*.mpeg";

        QFileInfoList files = storageDir.entryInfoList(filters, QDir::Files);

        int orphanedCount = 0;
        QStringList orphanedFiles;
        for (const QFileInfo &fileInfo : files)
        {
            if (!dbFilePaths.contains(fileInfo.absoluteFilePath()))
            {
                orphanedFiles.append(fileInfo.fileName());
                orphanedCount++;
            }
        }

        if (orphanedCount == 0)
        {
            QMessageBox::information(this, "Cleanup Complete", "No orphaned files found. Storage is clean!");
            return;
        }

        auto deleteReply = QMessageBox::question(
            this,
            "Orphaned Files Found",
            QString("Found %1 orphaned file(s):\n\n%2\n\nDelete these files?")
                .arg(orphanedCount)
                .arg(orphanedFiles.join("\n")),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (deleteReply == QMessageBox::Yes)
        {
            int deletedCount = 0;
            QStringList failedFiles;

            for (const QFileInfo &fileInfo : files)
            {
                if (!dbFilePaths.contains(fileInfo.absoluteFilePath()))
                {
                    QFile file(fileInfo.absoluteFilePath());
                    if (file.remove())
                        deletedCount++;
                    else
                        failedFiles.append(fileInfo.fileName());
                }
            }

            if (failedFiles.isEmpty())
            {
                QMessageBox::information(this, "Cleanup Complete",
                                         QString("Successfully deleted %1 orphaned file(s)!").arg(deletedCount));
            }
            else
            {
                QMessageBox::warning(this, "Cleanup Partial",
                                     QString("Deleted %1 file(s).\n\nFailed to delete:\n%2")
                                         .arg(deletedCount)
                                         .arg(failedFiles.join("\n")));
            }
        }
    }
    catch (const std::exception &e)
    {
        QMessageBox::warning(this, "Error", QString("Cleanup failed: %1").arg(e.what()));
    }
}

void MainTabs::openStorageFolder()
{
    QString path = QDir::toNativeSeparators(m_storageDirectory);

#ifdef Q_OS_WIN
    QProcess::startDetached("explorer", QStringList() << path);
#elif defined(Q_OS_MAC)
    QProcess::startDetached("open", QStringList() << path);
#else
    QProcess::startDetached("xdg-open", QStringList() << path);
#endif
}

void MainTabs::importFromExplorer()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select Media Files to Import",
        QDir::homePath(),
        "Media Files (*.m4a *.mp3 *.ogg *.flac *.wav *.mp4 *.mov *.avi *.mpg *.mpeg);;All Files (*)");

    if (filePaths.isEmpty())
        return;

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    for (const QString &filePath : filePaths)
    {
        QFileInfo fileInfo(filePath);
        QString newPath = QDir(m_storageDirectory).filePath(fileInfo.fileName());

        int counter = 1;
        while (QFile::exists(newPath))
        {
            QString baseName = fileInfo.baseName();
            QString suffix = fileInfo.suffix();
            newPath = QDir(m_storageDirectory).filePath(QString("%1_%2.%3").arg(baseName).arg(counter++).arg(suffix));
        }

        if (QFile::copy(filePath, newPath))
        {
            try
            {
                m_database->insertRecording(
                    newPath.toStdString(),
                    fileInfo.suffix().toUpper().toStdString(),
                    QFileInfo(newPath).fileName().toStdString());
                successCount++;
            }
            catch (const std::exception &e)
            {
                QFile::remove(newPath);
                failedFiles.append(fileInfo.fileName() + " (DB error)");
                failCount++;
            }
        }
        else
        {
            failedFiles.append(fileInfo.fileName() + " (Copy failed)");
            failCount++;
        }
    }

    if (failCount == 0)
    {
        QMessageBox::information(this, "Import Complete",
                                 QString("Successfully imported %1 file(s)!").arg(successCount));
    }
    else
    {
        QMessageBox::warning(this, "Import Partial",
                             QString("Imported %1 file(s).\n\nFailed to import %2 file(s):\n%3")
                                 .arg(successCount)
                                 .arg(failCount)
                                 .arg(failedFiles.join("\n")));
    }

    if (successCount > 0)
        refreshTable();
}

void MainTabs::exportToExplorer()
{
    int currentRow = m_table->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "No Selection", "Please select a recording to export.");
        return;
    }

    QString sourceFilePath = m_table->item(currentRow, 3)->text();
    QString fileName = m_table->item(currentRow, 1)->text();

    if (!QFile::exists(sourceFilePath))
    {
        QMessageBox::warning(this, "File Not Found",
                             "The source file no longer exists:\n" + sourceFilePath);
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Export Recording As",
        QDir::homePath() + "/" + fileName,
        "All Files (*)");

    if (savePath.isEmpty())
        return;

    if (QFile::exists(savePath))
        QFile::remove(savePath);

    if (QFile::copy(sourceFilePath, savePath))
    {
        QMessageBox::information(this, "Export Successful",
                                 "Recording exported successfully to:\n" + savePath);
    }
    else
    {
        QMessageBox::warning(this, "Export Failed",
                             "Failed to export recording to:\n" + savePath);
    }
}