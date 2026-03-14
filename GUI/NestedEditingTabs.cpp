#include "NestedEditingTabs.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QUrl>
#include <QPixmap>
#include <QImage>
#include <QVideoFrameFormat>

NestedEditingTabs::NestedEditingTabs(QWidget *parent, std::shared_ptr<SQLite> database, Mode mode)
    : QWidget(parent), m_database(database), m_mode(mode)
{
    setupUI();
    loadRecordings();
    updateSelectors();
}

NestedEditingTabs::~NestedEditingTabs()
{
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void NestedEditingTabs::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // File selector group
    QString fileSelectorTitle = (m_mode == Mode::Audio) ? "Select Audio File" : "Select Video File";
    QGroupBox *fileGroup = new QGroupBox(fileSelectorTitle);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);

    m_fileSelector = new QComboBox;
    connect(m_fileSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NestedEditingTabs::onFileSelected);
    fileLayout->addWidget(m_fileSelector);

    m_loadButton = new QPushButton("Load Selected File");
    m_loadButton->setEnabled(false);
    connect(m_loadButton, &QPushButton::clicked, this, &NestedEditingTabs::loadFile);
    fileLayout->addWidget(m_loadButton);

    layout->addWidget(fileGroup);

    // Preview group
    QGroupBox *previewGroup = new QGroupBox("Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    // Video-only: preview label above status
    if (m_mode == Mode::Video)
    {
        m_previewLabel = new QLabel("No file loaded");
        m_previewLabel->setAlignment(Qt::AlignCenter);
        m_previewLabel->setMinimumSize(640, 360);
        m_previewLabel->setMaximumSize(640, 360);
        m_previewLabel->setStyleSheet(
            "QLabel { background-color: black; color: white; border: 2px solid #ccc; }");
        previewLayout->addWidget(m_previewLabel);
    }

    m_statusLabel = new QLabel("No file loaded");
    m_statusLabel->setStyleSheet("font-size: 12pt;");
    previewLayout->addWidget(m_statusLabel);

    m_timeline = new QSlider(Qt::Horizontal);
    m_timeline->setEnabled(false);
    connect(m_timeline, &QSlider::sliderMoved, this, &NestedEditingTabs::onSliderMoved);
    previewLayout->addWidget(m_timeline);

    m_timeLabel = new QLabel("00:00 / 00:00");
    previewLayout->addWidget(m_timeLabel);

    QHBoxLayout *previewButtonLayout = new QHBoxLayout;

    m_playButton = new QPushButton("Play");
    m_playButton->setEnabled(false);
    connect(m_playButton, &QPushButton::clicked, this, &NestedEditingTabs::play);
    previewButtonLayout->addWidget(m_playButton);

    m_pauseButton = new QPushButton("Pause");
    m_pauseButton->setEnabled(false);
    connect(m_pauseButton, &QPushButton::clicked, this, &NestedEditingTabs::pause);
    previewButtonLayout->addWidget(m_pauseButton);

    previewLayout->addLayout(previewButtonLayout);
    layout->addWidget(previewGroup);

    // Edit operations group
    QGroupBox *editGroup = new QGroupBox("Edit Operations");
    QVBoxLayout *editLayout = new QVBoxLayout(editGroup);

    // Trim section
    QString trimTitle = (m_mode == Mode::Audio) ? "Trim Audio" : "Trim Video";
    QGroupBox *trimGroup = new QGroupBox(trimTitle);
    QFormLayout *trimLayout = new QFormLayout(trimGroup);

    m_trimStartEdit = new QLineEdit("00:00:00");
    m_trimStartEdit->setPlaceholderText("HH:MM:SS");
    trimLayout->addRow("Start Time:", m_trimStartEdit);

    m_trimEndEdit = new QLineEdit("00:00:00");
    m_trimEndEdit->setPlaceholderText("HH:MM:SS");
    trimLayout->addRow("End Time:", m_trimEndEdit);

    QString trimButtonLabel = (m_mode == Mode::Audio) ? "Trim Audio" : "Trim Video";
    m_trimButton = new QPushButton(trimButtonLabel);
    m_trimButton->setEnabled(false);
    m_trimButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 8px; }");
    connect(m_trimButton, &QPushButton::clicked, this, &NestedEditingTabs::trim);
    trimLayout->addRow(m_trimButton);

    editLayout->addWidget(trimGroup);

    // Extend section
    QString extendTitle = (m_mode == Mode::Audio) ? "Extend Audio (Concatenate)"
                                                  : "Extend Video (Concatenate)";
    QGroupBox *extendGroup = new QGroupBox(extendTitle);
    QVBoxLayout *extendLayout = new QVBoxLayout(extendGroup);

    extendLayout->addWidget(new QLabel("Select file to append:"));
    m_extendSelector = new QComboBox;
    extendLayout->addWidget(m_extendSelector);

    QString extendButtonLabel = (m_mode == Mode::Audio) ? "Extend Audio" : "Extend Video";
    m_extendButton = new QPushButton(extendButtonLabel);
    m_extendButton->setEnabled(false);
    m_extendButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
    connect(m_extendButton, &QPushButton::clicked, this, &NestedEditingTabs::extend);
    extendLayout->addWidget(m_extendButton);

    editLayout->addWidget(extendGroup);
    layout->addWidget(editGroup);

    // Save section
    QGroupBox *saveGroup = new QGroupBox("Save Edited File");
    QFormLayout *saveLayout = new QFormLayout(saveGroup);

    m_outputNameEdit = new QLineEdit;
    m_outputNameEdit->setPlaceholderText("Enter new filename");
    saveLayout->addRow("Output Name:", m_outputNameEdit);

    m_saveButton = new QPushButton("Save As");
    m_saveButton->setEnabled(false);
    m_saveButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    connect(m_saveButton, &QPushButton::clicked, this, &NestedEditingTabs::save);
    saveLayout->addRow(m_saveButton);

    layout->addWidget(saveGroup);
    layout->addStretch();
}

// ─── Data ─────────────────────────────────────────────────────────────────────

void NestedEditingTabs::loadRecordings()
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

void NestedEditingTabs::updateSelectors()
{
    loadRecordings();
    m_fileSelector->clear();
    m_extendSelector->clear();

    for (const auto &rec : m_recordings)
    {
        QString fileName = QString::fromStdString(rec->getFileName());
        QString format = QString::fromStdString(rec->getFileFormat()).toLower();

        bool matches = (m_mode == Mode::Audio)
                           ? (format == "m4a" || format == "mp3" || format == "ogg" ||
                              format == "flac" || format == "wav")
                           : (format == "mp4" || format == "mov" || format == "avi" ||
                              format == "mpg" || format == "mpeg");

        if (matches)
        {
            m_fileSelector->addItem(fileName);
            m_extendSelector->addItem(fileName);
        }
    }
}

// ─── File loading ─────────────────────────────────────────────────────────────

void NestedEditingTabs::onFileSelected(int index)
{
    m_loadButton->setEnabled(index >= 0);
}

void NestedEditingTabs::loadFile()
{
    int comboIndex = m_fileSelector->currentIndex();
    if (comboIndex < 0)
        return;

    QString selectedFileName = m_fileSelector->currentText();

    int actualIndex = -1;
    for (size_t i = 0; i < m_recordings.size(); ++i)
    {
        if (QString::fromStdString(m_recordings[i]->getFileName()) == selectedFileName)
        {
            actualIndex = static_cast<int>(i);
            break;
        }
    }

    if (actualIndex < 0)
        return;

    m_currentFilePath = QString::fromStdString(m_recordings[actualIndex]->getFilePath());

    if (!m_player)
    {
        m_player = new QMediaPlayer(this);
        m_audioOutput = new QAudioOutput(this);
        m_player->setAudioOutput(m_audioOutput);

        connect(m_player, &QMediaPlayer::durationChanged,
                this, &NestedEditingTabs::onDurationChanged);
        connect(m_player, &QMediaPlayer::positionChanged,
                this, &NestedEditingTabs::onPositionChanged);

        if (m_mode == Mode::Video)
        {
            m_videoSink = new QVideoSink(this);
            m_player->setVideoSink(m_videoSink);
            connect(m_videoSink, &QVideoSink::videoFrameChanged,
                    this, &NestedEditingTabs::updateVideoFrame);
        }
    }

    m_player->setSource(QUrl::fromLocalFile(m_currentFilePath));

    m_statusLabel->setText("Loaded: " + QString::fromStdString(m_recordings[actualIndex]->getFileName()));
    m_playButton->setEnabled(true);
    m_pauseButton->setEnabled(true);
    m_timeline->setEnabled(true);
    m_trimButton->setEnabled(true);
    m_extendButton->setEnabled(true);
    m_saveButton->setEnabled(true);
}

// ─── Playback ─────────────────────────────────────────────────────────────────

void NestedEditingTabs::play()
{
    if (m_player)
        m_player->play();
}

void NestedEditingTabs::pause()
{
    if (m_player)
        m_player->pause();
}

void NestedEditingTabs::onDurationChanged(qint64 duration)
{
    m_timeline->setMaximum(duration);
    qint64 seconds = duration / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    m_timeLabel->setText("00:00 / " + QString("%1:%2:%3")
                                          .arg(hours, 2, 10, QChar('0'))
                                          .arg(minutes % 60, 2, 10, QChar('0'))
                                          .arg(seconds % 60, 2, 10, QChar('0')));
}

void NestedEditingTabs::onPositionChanged(qint64 position)
{
    if (!m_timeline->isSliderDown())
        m_timeline->setValue(position);

    qint64 seconds = position / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;

    QString positionStr = QString("%1:%2:%3")
                              .arg(hours, 2, 10, QChar('0'))
                              .arg(minutes % 60, 2, 10, QChar('0'))
                              .arg(seconds % 60, 2, 10, QChar('0'));

    int slashPos = m_timeLabel->text().indexOf('/');
    if (slashPos != -1)
        m_timeLabel->setText(positionStr + " " + m_timeLabel->text().mid(slashPos));
}

void NestedEditingTabs::onSliderMoved(int position)
{
    if (m_player)
        m_player->setPosition(position);
}

// ─── Edit operations ──────────────────────────────────────────────────────────

void NestedEditingTabs::trim()
{
    QString type = (m_mode == Mode::Audio) ? "Audio" : "Video";
    QString inExt = (m_mode == Mode::Audio) ? "m4a" : "mp4";
    QString outExt = inExt;

    QString body = "Trimming requires FFmpeg integration.\n\n"
                   "This would execute:\n"
                   "ffmpeg -i input." +
                   inExt +
                   " -ss " + m_trimStartEdit->text() +
                   " -to " + m_trimEndEdit->text() +
                   " -c copy output." + outExt + "\n\n"
                                                 "Please install FFmpeg to enable this feature.";

    QMessageBox::information(this, "Trim " + type, body);
}

void NestedEditingTabs::extend()
{
    QString type = (m_mode == Mode::Audio) ? "Audio" : "Video";

    if (m_mode == Mode::Audio)
    {
        QMessageBox::information(this, "Extend Audio",
                                 "Audio concatenation requires FFmpeg integration.\n\n"
                                 "This would execute:\n"
                                 "ffmpeg -i concat:file1.m4a|file2.m4a -c copy output.m4a\n\n"
                                 "Please install FFmpeg to enable this feature.");
    }
    else
    {
        QMessageBox::information(this, "Extend Video",
                                 "Video concatenation requires FFmpeg integration.\n\n"
                                 "This would create a concat list file and execute:\n"
                                 "ffmpeg -f concat -safe 0 -i filelist.txt -c copy output.mp4\n\n"
                                 "Please install FFmpeg to enable this feature.");
    }
}

void NestedEditingTabs::save()
{
    QString fileName = m_outputNameEdit->text();
    if (fileName.isEmpty())
    {
        QMessageBox::warning(this, "Invalid Name", "Please enter an output filename.");
        return;
    }

    QString type = (m_mode == Mode::Audio) ? "audio" : "video";
    QMessageBox::information(this, "Save",
                             "Edited " + type + " would be saved as: " + fileName + "\n\n"
                                                                                    "This feature requires FFmpeg processing implementation.");
}

// ─── Video-only ───────────────────────────────────────────────────────────────

void NestedEditingTabs::updateVideoFrame(const QVideoFrame &frame)
{
    if (!m_previewLabel)
        return;

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QVideoFrame::ReadOnly))
        return;

    QImage img = cloneFrame.toImage();
    cloneFrame.unmap();

    if (img.isNull())
        return;

    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_RGB32);

    m_previewLabel->setPixmap(
        QPixmap::fromImage(img).scaled(m_previewLabel->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
}