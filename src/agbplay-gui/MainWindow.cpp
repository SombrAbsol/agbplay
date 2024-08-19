#include "MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QToolBar>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDesktopServices>

#include <fmt/core.h>
#include <thread>
#include <format>
#include <fstream>
#include <algorithm>
#include <tuple>

#include "SelectProfileDialog.h"
#include "AboutWindow.h"

#include "ProfileManager.h"
#include "Rom.h"
#include "PlaybackEngine.h"
#include "SoundExporter.h"
#include "Debug.h"
#include "FileReader.h"
#include "Gsf.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    SetupMenuBar();
    SetupToolBar();
    SetupWidgets();
    SetupStatusBar();

    pm = std::make_unique<ProfileManager>();
    pm->LoadProfiles();
    connect(&statusUpdateTimer, &QTimer::timeout, this, &MainWindow::StatusUpdate);
    statusUpdateTimer.setTimerType(Qt::PreciseTimer);
    statusUpdateTimer.setInterval(16);
    statusUpdateTimer.start();

    Debug::set_callback(&MainWindow::LogCallback, static_cast<void *>(this));

    connect(&songlistWidget.addRemoveButton, &QAbstractButton::clicked, [this](bool) { PlaylistAdd(); });
    connect(&playlistWidget.addRemoveButton, &QAbstractButton::clicked, [this](bool) { PlaylistRemove(); });

    connect(&songlistWidget, &SonglistWidget::ContextMenuActionAdd, [this]() { PlaylistAdd(); });
    connect(&playlistWidget, &SonglistWidget::ContextMenuActionRemove, [this]() { PlaylistRemove(); });

    connect(&playlistWidget, &SonglistWidget::ContentChanged, [this]() {
        if (!profile)
            return;
        profile->dirty = true;
        saveButton.setEnabled(true);
        saveProfileAction->setEnabled(true);
    });

    setWindowTitle("agbplay");
}

MainWindow::~MainWindow()
{
    Debug::set_callback(nullptr, nullptr);
}

void MainWindow::SetupMenuBar()
{
    /* File */
    QMenu *fileMenu = menuBar()->addMenu("&File");

    QAction *fileOpenRom = fileMenu->addAction("Open ROM/GSF");
    fileOpenRom->setIcon(QIcon(":/icons/open-rom.ico"));
    fileOpenRom->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_O));
    connect(fileOpenRom, &QAction::triggered, [this](bool){ LoadGame(); });

    fileMenu->addSeparator();

    saveProfileAction = fileMenu->addAction("Save Profile");
    saveProfileAction->setIcon(QIcon(":/icons/profile-save.ico"));
    saveProfileAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_S));
    saveProfileAction->setEnabled(false);
    connect(saveProfileAction, &QAction::triggered, [this](bool){ SaveProfile(); });

    exportAudioAction = fileMenu->addAction("Export Audio");
    exportAudioAction->setIcon(QIcon(":/icons/export-audio.ico"));
    exportAudioAction->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_E));
    exportAudioAction->setEnabled(false);
    connect(exportAudioAction, &QAction::triggered, [this](bool){ ExportAudio(false, false); });

    QAction *exportMidi = fileMenu->addAction("Export MIDI");
    exportMidi->setIcon(QIcon(":/icons/export-midi.ico"));
    exportMidi->setEnabled(false);

    fileMenu->addSeparator();

    QAction *fileQuit = fileMenu->addAction("Quit");
    fileQuit->setIcon(QIcon(":/icons/exit.ico"));
    fileQuit->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Q));

    connect(fileQuit, &QAction::triggered, [this](bool) { close(); });
    fileQuit->setMenuRole(QAction::QuitRole);

    /* Edit */
    QMenu *editMenu = menuBar()->addMenu("&Edit");
    QAction *editPreferences = editMenu->addAction("Global Preferences");
    editPreferences->setIcon(QIcon(":/icons/preferences.ico"));

    /* Profile */
    QMenu *profileMenu = menuBar()->addMenu("&Profile");
    QAction *profileSelect = profileMenu->addAction("Load Profile");
    profileSelect->setIcon(QIcon(":/icons/profile-select.ico"));
    profileSelect->setEnabled(false);
    QAction *profileCreate = profileMenu->addAction("Create New Profile");
    profileCreate->setIcon(QIcon(":/icons/profile-create.ico"));
    profileCreate->setEnabled(false);
    QAction *profileDuplicate = profileMenu->addAction("Duplicate Current Profile");
    profileDuplicate->setIcon(QIcon(":/icons/profile-duplicate.ico"));
    profileDuplicate->setEnabled(false);
    QAction *profileDelete = profileMenu->addAction("Delete Current Profile");
    profileDelete->setIcon(QIcon(":/icons/profile-delete.ico"));
    profileDelete->setEnabled(false);
    profileMenu->addSeparator();
    QAction *profileSettings = profileMenu->addAction("Profile Settings");
    profileSettings->setIcon(QIcon(":/icons/profile-settings.ico"));
    profileSettings->setEnabled(false);
    profileMinigsfImport = profileMenu->addAction("Import GSF Playlist");
    profileMinigsfImport->setIcon(QIcon(":/icons/profile-import-minigsf.ico"));
    profileMinigsfImport->setEnabled(false);
    connect(profileMinigsfImport, &QAction::triggered, [this](bool){ ProfileImportGsfPlaylist(); });
    profileMenu->addSeparator();
    QAction *profileDirectory = profileMenu->addAction("Open User Profile Directory");
    profileDirectory->setIcon(QIcon(":/icons/profile-open-folder.ico"));
    connect(profileDirectory, &QAction::triggered, [this](bool) {
            QDesktopServices::openUrl(QString::fromStdWString(ProfileManager::ProfileUserPath().wstring()));
    });

    /* Help */
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *helpSaveLog = helpMenu->addAction("Save Log");
    helpSaveLog->setIcon(QIcon(":/icons/save-log.ico"));
    connect(helpSaveLog, &QAction::triggered, [this](bool){ SaveLog(); });
    helpMenu->addSeparator();
    QAction *helpAboutAction = helpMenu->addAction("About");
    helpAboutAction->setIcon(QIcon(":/icons/about.ico"));
    connect(helpAboutAction, &QAction::triggered, [this](bool) {
            AboutWindow w(this);
            w.exec();
    });
    helpAboutAction->setMenuRole(QAction::AboutRole);
}

void MainWindow::SetupToolBar()
{
    QToolBar *toolBar = addToolBar("HAHAHAA");

    saveButton.setIcon(QIcon(":/icons/save-profile-large.ico"));
    saveButton.setFixedSize(32, 32);
    saveButton.setIconSize(QSize(32, 32));
    saveButton.setEnabled(false);
    toolBar->addWidget(&saveButton);
    connect(&saveButton, &QAbstractButton::clicked, [this](bool) { SaveProfile(); });

    toolBar->addSeparator();

    stopButton.setIcon(QIcon(":/icons/playback-stop.ico"));
    stopButton.setFixedSize(32, 32);
    stopButton.setIconSize(QSize(32, 32));
    stopButton.setCheckable(true);
    stopButton.setChecked(true);
    toolBar->addWidget(&stopButton);
    connect(&stopButton, &QAbstractButton::clicked, [this](bool) { Stop(); });

    playButton.setIcon(QIcon(":/icons/playback-play.ico"));
    playButton.setFixedSize(32, 32);
    playButton.setIconSize(QSize(32, 32));
    playButton.setCheckable(true);
    toolBar->addWidget(&playButton);
    connect(&playButton, &QAbstractButton::clicked, [this](bool) { Play(); });

    pauseButton.setIcon(QIcon(":/icons/playback-pause.ico"));
    pauseButton.setFixedSize(32, 32);
    pauseButton.setIconSize(QSize(32, 32));
    pauseButton.setCheckable(true);
    toolBar->addWidget(&pauseButton);
    connect(&pauseButton, &QAbstractButton::clicked, [this](bool) { Pause(); });

    prevButton.setIcon(QIcon(":/icons/playback-previous.ico"));
    prevButton.setFixedSize(32, 32);
    prevButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&prevButton);
    connect(&prevButton, &QAbstractButton::clicked, [this](bool) { AdvanceSong(-1); });

    nextButton.setIcon(QIcon(":/icons/playback-next.ico"));
    nextButton.setFixedSize(32, 32);
    nextButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&nextButton);
    connect(&nextButton, &QAbstractButton::clicked, [this](bool) { AdvanceSong(1); });

    toolBar->addSeparator();

    speedHalveButton.setIcon(QIcon(":/icons/speed-halve.ico"));
    speedHalveButton.setFixedSize(32, 32);
    speedHalveButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&speedHalveButton);
    connect(&speedHalveButton, &QAbstractButton::clicked, [this](bool) { SpeedHalve(); });

    speedDoubleButton.setIcon(QIcon(":/icons/speed-double.ico"));
    speedDoubleButton.setFixedSize(32, 32);
    speedDoubleButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&speedDoubleButton);
    connect(&speedDoubleButton, &QAbstractButton::clicked, [this](bool) { SpeedDouble(); });

    toolBar->addSeparator();

    toolBar->addWidget(&vuMeter);
}

void MainWindow::SetupWidgets()
{
    /* 1. Create containers. */
    containerCentralLayout.addWidget(&containerCentralSplitter);
    containerCentralLayout.setContentsMargins(0, 0, 0, 0);
    containerCentralSplitter.setChildrenCollapsible(false);
    containerCentralSplitter.addWidget(&containerLeft);
    containerCentralSplitter.addWidget(&containerRight);
    containerCentralSplitter.setStretchFactor(0, 1);
    containerCentralSplitter.setStretchFactor(1, 4);
    setCentralWidget(&containerCentral);

    /* 2. Create songlist and playlist. */
    containerLeftLayout.addWidget(&songlistWidget);
    connect(&songlistWidget.listWidget, &QAbstractItemView::doubleClicked, [this](const QModelIndex &index) {
            const QListWidgetItem *item = songlistWidget.listWidget.item(index.row());
            assert(item);
            if (!item)
                return;
            playlistFocus = false;
            LoadSong(item->text().toStdString(), static_cast<uint16_t>(item->data(Qt::UserRole).toUInt()));
            songlistWidget.SelectSong(index.row());
            Play();
    });
    containerLeftLayout.addWidget(&playlistWidget);
    connect(&playlistWidget.listWidget, &QAbstractItemView::doubleClicked, [this](const QModelIndex &index) {
            const QListWidgetItem *item = playlistWidget.listWidget.item(index.row());
            assert(item);
            if (!item)
                return;
            playlistFocus = true;
            LoadSong(item->text().toStdString(), static_cast<uint16_t>(item->data(Qt::UserRole).toUInt()));
            playlistWidget.SelectSong(index.row());
            Play();
    });
    containerLeftLayout.setContentsMargins(0, 0, 0, 0);

    /* 3. Create rom info and main status view. */
    containerStatusInfoLayout.addWidget(&containerStatusInfoSplitter);
    containerStatusInfoLayout.setContentsMargins(0, 0, 0, 0);
    containerStatusInfoSplitter.setChildrenCollapsible(false);
    containerStatusInfoSplitter.addWidget(&statusWidgetScrollArea);
    containerStatusInfoSplitter.addWidget(&infoWidget);
    containerStatusInfoSplitter.setStretchFactor(0, 5);
    containerStatusInfoSplitter.setStretchFactor(1, 1);
    statusWidgetScrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    statusWidgetScrollArea.setWidgetResizable(true);
    statusWidgetScrollArea.setWidget(&statusWidget);
    infoWidget.setMaximumWidth(250);

    /* 4. Create log */
    containerRightLayout.addWidget(&containerRightSplitter);
    containerRightSplitter.setChildrenCollapsible(false);
    containerRightSplitter.addWidget(&containerStatusInfo);
    containerRightSplitter.addWidget(&logWidget);
    containerRightSplitter.setStretchFactor(0, 5);
    containerRightSplitter.setStretchFactor(1, 1);
    containerRightLayout.setContentsMargins(0, 0, 0, 0);
    logWidget.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    logWidget.setFont(QFont("Monospace")); // TODO does this work on windows?
    logWidget.setAcceptRichText(false);
    LogAppend("agbplay ready!");

    centralWidget()->show();
}

void MainWindow::SetupStatusBar()
{
    statusBar()->showMessage("No game loaded");
    statusBar()->addPermanentWidget(&progressBar);
    progressBar.setMaximumHeight(16);
    progressBar.setMinimumHeight(16);
    progressBar.setMaximumWidth(128);
    progressBar.setMinimumWidth(128);
    progressBar.setRange(0, 100);
    progressBar.setValue(20);
    progressBar.hide(); // progress bar is only shown on demand
}

void MainWindow::Play()
{
    if (!playbackEngine) {
        playButton.setChecked(false);
        return;
    }

    playbackEngine->Play();
    playing = true;

    if (playlistFocus)
        playlistWidget.SetPlayState(true);
    else
        songlistWidget.SetPlayState(true);

    playButton.setChecked(true);
    stopButton.setChecked(false);
    pauseButton.setChecked(false);
}

void MainWindow::Pause()
{
    if (!playbackEngine) {
        pauseButton.setChecked(false);
        return;
    }

    playing = playbackEngine->Pause();

    playButton.setChecked(playing);
    stopButton.setChecked(false);
    pauseButton.setChecked(!playing);
}

void MainWindow::Stop()
{
    if (!playbackEngine) {
        stopButton.setChecked(false);
        return;
    }

    playbackEngine->Stop();
    playing = false;

    if (playlistFocus)
        playlistWidget.SetPlayState(false);
    else
        songlistWidget.SetPlayState(false);

    playButton.setChecked(false);
    stopButton.setChecked(true);
    pauseButton.setChecked(false);
}

void MainWindow::AdvanceSong(int indexDelta)
{
    if (!playbackEngine)
        return;

    SonglistWidget &listWidget = playlistFocus ? playlistWidget : songlistWidget;
    const int oldIndex = listWidget.GetSelectedSong();
    const int newIndex = oldIndex + indexDelta;

    QListWidgetItem *oldItem = listWidget.listWidget.item(oldIndex);
    QListWidgetItem *newItem = listWidget.listWidget.item(newIndex);

    if (oldItem == nullptr)
        return;

    if (newItem == nullptr) {
        Stop();
    } else {
        listWidget.SelectSong(newIndex);
        LoadSong(newItem->text().toStdString(), static_cast<uint16_t>(newItem->data(Qt::UserRole).toUInt()));
    }
}

void MainWindow::LoadSong(const std::string &title, uint16_t id)
{
    if (!playbackEngine)
        return;

    playbackEngine->LoadSong(id);
    statusWidget.songWidget.titleLabel.setText(QString::fromStdString(fmt::format("{} - {}", id, title)));

    if (playlistFocus) {
        songlistWidget.SetPlayState(false);
        playlistWidget.SetPlayState(playing);
    } else {
        songlistWidget.SetPlayState(playing);
        playlistWidget.SetPlayState(false);
    }

    if (playing)
        playbackEngine->Play();
}

void MainWindow::SpeedHalve()
{
    if (!playbackEngine)
        return;

    playbackEngine->SpeedHalve();
}

void MainWindow::SpeedDouble()
{
    if (!playbackEngine)
        return;

    playbackEngine->SpeedDouble();
}

void MainWindow::PlaylistAdd()
{
    QList<QListWidgetItem *> items = songlistWidget.listWidget.selectedItems();
    for (int i = 0; i < items.count(); i++) {
        QListWidgetItem *item = items.at(i);
        if (!item)
            continue;
        const std::string name = item->text().toStdString();
        const uint16_t id = static_cast<uint16_t>(item->data(Qt::UserRole).toUInt());
        playlistWidget.AddSong(name, id);
    }
}

void MainWindow::PlaylistRemove()
{
    playlistWidget.RemoveSong();
}

void MainWindow::ProfileImportGsfPlaylist(const std::filesystem::path &gameFilePath)
{
    if (!profile)
        return;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setNameFilter("MINIGSF files (*.minigsf *.zip)");

    std::vector<std::filesystem::path> pathsToLoad;

    auto miniGsfFilterFunc = [](const std::filesystem::path &p) {
        return FileReader::cmpPathExt(p, "minigsf");
    };

    if (gameFilePath.empty()) {
        /* If no path is passed, ask the user to specify them interactively */
        if (!fileDialog.exec())
            return;
        for (int i = 0; i < fileDialog.selectedFiles().size(); i++)
            pathsToLoad.emplace_back(fileDialog.selectedFiles().at(i).toStdWString());
    } else {
        if (FileReader::cmpPathExt(gameFilePath, "gsflib")) {
            /* Main file loaded from raw file -> load minigsfs from same folder */
            for (const auto &dirEnt : std::filesystem::directory_iterator(gameFilePath.parent_path())) {
                if (!dirEnt.is_regular_file())
                    continue;
                if (miniGsfFilterFunc(dirEnt.path()))
                    pathsToLoad.emplace_back(dirEnt.path());
            }
        } else if (FileReader::cmpPathExt(gameFilePath, "zip")) {
            /* Main file loaded from zip file -> load minigsfs from same zip file */
            pathsToLoad.emplace_back(gameFilePath);
        } else {
            throw std::logic_error("This case should not occur. Attempting to load MINIGSFs from invalid main file extension.");
        }
    }

    /* Store all songs together with their original file name (for sorting later) */
    std::vector<std::tuple<std::filesystem::path, std::string, uint16_t>> songs;

    auto op = [&songs](const std::filesystem::path &p, FileReader &fileReader) {
        std::string title;
        uint16_t id;
        std::vector<uint8_t> gsfData(fileReader.size());
        fileReader.read(gsfData);
        Gsf::GetSongInfo(gsfData, title, id);
        songs.emplace_back(p, std::move(title), id);
        return true;
    };

    /* FileReader::forEachIn will run 'op' if 'miniGsfFilterFunc' returns true
     * for a specific path. In our case 'op' will populate our song list to load. */
    for (const std::filesystem::path &p : pathsToLoad)
        FileReader::forEachInZipOrRaw(p, miniGsfFilterFunc, op);

    /* Playlist order may be random, so sort by filename like normal for MINIGSFs. */
    auto pathCmp = [](const auto &ta, const auto &tb){
        return std::get<0>(ta).stem() < std::get<0>(tb).stem();
    };
    std::sort(songs.begin(), songs.end(), pathCmp);

    /* Ask the user to replace all current songs if there are any */
    if (playlistWidget.listWidget.count() > 0) {
        const QString title = "Overwrite exiting playlist?";
        QString message = "The playlist of the current profile already contains songs. ";
        message += "Do you want to keep the current songs in the playlist before import?";
        QMessageBox mbox(QMessageBox::Icon::Question, title, message, QMessageBox::Yes | QMessageBox::No, this);
        if (mbox.exec() == QMessageBox::No)
            playlistWidget.listWidget.clear();
    }

    for (auto &[_, title, id] : songs)
        playlistWidget.AddSong(title, id);

    if (songs.size() > 0)
        profile->dirty = true;
}

void MainWindow::LoadGame()
{
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter("GBA-ROMs (*.gba *.zip);;GSF-Lib (*.gsflib *.zip)");

    if (!fileDialog.exec())
        return;

    assert(fileDialog.selectedFiles().size() == 1);

    if (!CloseGame())
        return;

    std::vector<std::shared_ptr<Profile>> profileCandidates;

    try {
        Rom::CreateInstance(fileDialog.selectedFiles().at(0).toStdWString());
        MP2KScanner scanner(Rom::Instance());
        profileCandidates = pm->GetProfiles(Rom::Instance(), scanner.Scan());
    } catch (Xcept &e) {
        MBoxError("Load Error", e.what());
        return;
    }

    if (profileCandidates.size() > 1) {
        SelectProfileDialog profileDialog;
        for (const std::shared_ptr<Profile> &profile : profileCandidates)
            profileDialog.addToSelectionDialog(*profile);

        if (!profileDialog.exec())
            return;

        assert(profileDialog.selectedProfile() >= 0);

        profile = profileCandidates.at(profileDialog.selectedProfile());
    } else {
        profile = profileCandidates.at(0);
    }

    for (uint16_t i = 0; i < profile->songTableInfoPlayback.count; i++) {
        auto songName = fmt::format("{:04}", i);
        songlistWidget.AddSong(songName, i);
    }

    for (size_t i = 0; i < profile->playlist.size(); i++) {
        playlistWidget.AddSong(profile->playlist.at(i).name, profile->playlist.at(i).id);
    }

    /* Inserting songs into the playlistWidget falsely marks the profile as dirty.
     * Undo this. */
    profile->dirty = false;
    saveButton.setEnabled(false);
    saveProfileAction->setEnabled(false);

    if (Rom::Instance().IsGsf() && profile->playlist.size() == 0) {
        const QString title = "Load Playlist from GSF set?";
        QString message = "You just loaded a GSF set. ";
        message += "The current profile does not contain any songs in the playlist.\n\n";
        message += "Do you want to import the playlist from the accompanying MINIGSFs?";
        QMessageBox mbox(QMessageBox::Icon::Question, title, message, QMessageBox::Yes | QMessageBox::No, this);
        if (mbox.exec() == QMessageBox::Yes)
            ProfileImportGsfPlaylist(fileDialog.selectedFiles().at(0).toStdWString());
    }

    infoWidget.romNameLineEdit.setText(QString::fromStdString(Rom::Instance().ReadString(0xA0, 12)));
    infoWidget.romCodeLineEdit.setText(QString::fromStdString(Rom::Instance().GetROMCode()));
    infoWidget.songTableLineEdit.setText(QString::fromStdString(fmt::format("0x{:X}", profile->songTableInfoPlayback.pos)));
    infoWidget.songCountLineEdit.setText(QString::number(profile->songTableInfoPlayback.count));

    playbackEngine = std::make_unique<PlaybackEngine>(*profile);
    visualizerState = std::make_unique<MP2KVisualizerState>();

    exportAudioAction->setEnabled(true);
    profileMinigsfImport->setEnabled(true);
}

bool MainWindow::CloseGame()
{
    if (int result = AskSaveProfile(); result == QMessageBox::Cancel)
        return false;
    else if (result == QMessageBox::Save)
        SaveProfile();
    else
        DiscardProfile();
    Stop();
    playbackEngine.reset();
    profile.reset();

    statusWidget.reset();
    songlistWidget.Clear();
    playlistWidget.Clear();
    exportAudioAction->setEnabled(false);
    profileMinigsfImport->setEnabled(false);
    return true;
}

void MainWindow::ExportAudio(bool benchmarkOnly, bool separateTracks)
{
    /* Check if export is currently going on and join thread afterwards. */
    if (exportBusy) {
        ExportStillInProgress();
        return;
    }

    if (!profile)
        return;

    if (exportThread) {
        exportThread->join();
        exportThread.reset();
    }

    /* Select directory to export to. */
    QFileDialog fileDialog{this};
    fileDialog.setFileMode(QFileDialog::Directory);
    fileDialog.setDirectory(QString::fromStdWString(SoundExporter::DefaultDirectory().wstring()));

    if (!fileDialog.exec())
        return;

    assert(fileDialog.selectedFiles().size() == 1);

    const std::filesystem::path directory = fileDialog.selectedFiles().at(0).toStdWString();

    /* Make a Profile copy for use in export thread.
     * That way we do not overwrite it while exporting. */
    Profile profileToExport = *profile;

    /* Modify export profile to only contain selected songs. */
    profileToExport.playlist.clear();

    for (int i = 0; i < songlistWidget.listWidget.count(); i++) {
        QListWidgetItem *item = songlistWidget.listWidget.item(i);
        if (!item)
            continue;

        const std::string name = item->text().toStdString();
        const uint16_t id = static_cast<uint16_t>(item->data(Qt::UserRole).toUInt());
        if (item->checkState() == Qt::Checked)
            profileToExport.playlist.emplace_back(name, id);
    }

    for (int i = 0; i < playlistWidget.listWidget.count(); i++) {
        QListWidgetItem *item = playlistWidget.listWidget.item(i);
        if (!item)
            continue;

        const std::string name = item->text().toStdString();
        const uint16_t id = static_cast<uint16_t>(item->data(Qt::UserRole).toUInt());
        if (item->checkState() == Qt::Checked)
            profileToExport.playlist.emplace_back(name, id);
    }

    exportBusy = true;

    // TODO implement benchmark flag
    // TODO implement progress bar
    exportThread = std::make_unique<std::thread>([this](std::filesystem::path tDirectory, Profile tProfile, bool tBenchmarkOnly, bool tSeparateTracks) {
            SoundExporter se(tDirectory, tProfile, tBenchmarkOnly, tSeparateTracks);
            se.Export();
            exportBusy = false;
        },
        directory,
        profileToExport,
        benchmarkOnly,
        separateTracks
    );
#ifdef __linux__
    pthread_setname_np(exportThread->native_handle(), "export thread");
#endif
}

void MainWindow::ExportStillInProgress()
{
    MBoxError(
        "Export in progress",
        "There is already an export in progress. Please wait for the current export to complete."
    );
}

void MainWindow::SaveLog()
{
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setNameFilter("Log File (*.log *.txt)");
    fileDialog.setDefaultSuffix("log");

    if (!fileDialog.exec())
        return;

    assert(fileDialog.selectedFiles().size() == 1);

    const std::filesystem::path selectedFile = fileDialog.selectedFiles().at(0).toStdWString();
    std::ofstream fileStream(selectedFile);
    if (!fileStream.is_open()) {
        MBoxError("Save log file failed", fmt::format("Failed to save file: {}", strerror(errno)));
        return;
    }

    fileStream << logWidget.toPlainText().toStdString();
}

void MainWindow::SaveProfile()
{
    if (!profile || !profile->dirty)
        return;

    profile->playlist.clear();
    for (int i = 0; i < playlistWidget.listWidget.count(); i++) {
        QListWidgetItem *item = playlistWidget.listWidget.item(i);
        if (!item)
            continue;

        const std::string title = item->text().toStdString();
        const uint16_t id = static_cast<uint16_t>(item->data(Qt::UserRole).toUInt());
        profile->playlist.emplace_back(title, id);
    }

    saveButton.setEnabled(false);
    saveProfileAction->setEnabled(false);
    assert(pm);
    pm->SaveProfiles();
}

void MainWindow::DiscardProfile()
{
    if (!profile)
        return;

    saveButton.setEnabled(false);
    saveProfileAction->setEnabled(false);
    profile->dirty = false;
}

int MainWindow::AskSaveProfile()
{
    if (!profile || !profile->dirty)
        return QMessageBox::Discard;

    const QString title = "Save changed profile?";
    const QString message = "The currentl profile has unsaved changes. Do you want to save the changes?";
    QMessageBox mbox(QMessageBox::Icon::Question, title, message, QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard, this);
    return mbox.exec();
}

void MainWindow::MBoxInfo(const std::string &title, const std::string &msg)
{
    const QString qtitle = QString::fromStdString(title);
    const QString qmsg = QString::fromStdString(msg);
    QMessageBox mbox(QMessageBox::Icon::Information, qtitle, qmsg, QMessageBox::Ok, this);
    mbox.exec();
}

void MainWindow::MBoxError(const std::string &title, const std::string &msg)
{
    const QString qtitle = QString::fromStdString(title);
    const QString qmsg = QString::fromStdString(msg);
    QMessageBox mbox(QMessageBox::Icon::Critical, qtitle, qmsg, QMessageBox::Ok, this);
    mbox.exec();
}

void MainWindow::StatusUpdate()
{
    if (!playbackEngine || !visualizerState)
        return;

    playbackEngine->GetVisualizerState(*visualizerState);
    vuMeter.SetLevel(visualizerState->masterVolLeft, visualizerState->masterVolRight, 1.0f, 1.0f);
    statusWidget.setVisualizerState(*visualizerState);

    if (playing && playbackEngine->HasEnded()) {
        /* AdvanceSong automatically stops if this was the last song in the list. */
        AdvanceSong(1);
    }
}

void MainWindow::LogCallback(const std::string &msg, void *void_this)
{
    MainWindow *_this = static_cast<MainWindow *>(void_this);
    // Qt 6.7 only
    //QMetaObject::invokeMethod(_this, &MainWindow::LogAppend, Qt::QueuedConnection, msg);
    QMetaObject::invokeMethod(_this, "LogAppend", Qt::QueuedConnection, msg);
}

void MainWindow::LogAppend(std::string msg)
{
    const std::string final_msg = std::format("[{:%T}] {}", std::chrono::system_clock::now(), msg);
    logWidget.append(QString::fromStdString(final_msg));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (exportBusy) {
        ExportStillInProgress();
        event->ignore();
        return;
    }

    if (exportThread) {
        exportThread->join();
        exportThread.reset();
    }

    if (CloseGame())
        event->accept();
    else
        event->ignore();
}
