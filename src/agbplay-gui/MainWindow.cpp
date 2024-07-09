#include "MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    Setup();
}

MainWindow::~MainWindow()
{
}

void MainWindow::Setup()
{
    /* Status Bar */
    statusBar()->showMessage("HELLO");

    /* Menu Bar - File */
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *fileOpenAction = fileMenu->addAction("Open");
    fileMenu->addSeparator();
    QAction *fileQuitAction = fileMenu->addAction("Quit");
    connect(fileQuitAction, &QAction::triggered, [this](bool) { close(); });
    fileQuitAction->setMenuRole(QAction::QuitRole);

    /* Menu Bar - Edit */
    QMenu *editMenu = menuBar()->addMenu("Edit");
    QAction *editPreferences = editMenu->addAction("Global Preferences");

    /* Menu Bar - Profile */
    QMenu *profileMenu = menuBar()->addMenu("Profile");
    QAction *profileSelect = profileMenu->addAction("Load Profile");
    profileSelect->setEnabled(false);
    QAction *profileCreate = profileMenu->addAction("Create New Profile");
    profileCreate->setEnabled(false);
    QAction *profileDuplicate = profileMenu->addAction("Duplicate Current Profile");
    profileDuplicate->setEnabled(false);
    QAction *profileDelete = profileMenu->addAction("Delete Current Profile");
    profileDelete->setEnabled(false);
    QAction *profileSettings = profileMenu->addAction("Profile Settings");
    profileSettings->setEnabled(false);
    profileMenu->addSeparator();
    QAction *profileDirectory = profileMenu->addAction("Open User Profile Directory");

    /* Menu Bar - Help */
    QMenu *helpMenu = menuBar()->addMenu("Help");
    QAction *helpSaveLog = helpMenu->addAction("Save Log");
    QAction *helpAboutAction = helpMenu->addAction("About");
    connect(helpAboutAction, &QAction::triggered, [this](bool) {
        QMessageBox mbox(QMessageBox::Icon::Information, "TEST", "TEST 2", QMessageBox::Ok, this);
        mbox.exec();
    });
    helpAboutAction->setMenuRole(QAction::AboutRole);

    /* 1. Create containers. */
    containerCentralLayout.addWidget(&containerLeft);
    containerCentralLayout.addWidget(&containerRight);
    containerCentralLayout.setStretch(0, 1);
    containerCentralLayout.setStretch(1, 5);
    containerCentralLayout.setContentsMargins(0, 0, 0, 0);
    setCentralWidget(&containerCentral);

    /* 2. Create songlist and playlist. */
    containerLeftLayout.addWidget(&songlistWidget);
    containerLeftLayout.addWidget(&playlistWidget);
    containerLeftLayout.setContentsMargins(0, 0, 0, 0);

    /* 3. Create rom info and main status view. */
    containerStatusInfoLayout.addWidget(&statusWidget);
    containerStatusInfoLayout.addWidget(&infoWidget);
    containerStatusInfoLayout.setStretch(0, 5);
    containerStatusInfoLayout.setStretch(1, 1);
    containerStatusInfoLayout.setContentsMargins(0, 0, 0, 0);

    /* 4. Create log */
    containerRightLayout.addWidget(&containerStatusInfo);
    containerRightLayout.addWidget(&logWidget);
    containerRightLayout.setStretch(0, 5);
    containerRightLayout.setStretch(1, 1);
    containerRightLayout.setContentsMargins(0, 0, 0, 0);
    logWidget.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    logWidget.setFont(QFont("Monospace"));
    logWidget.setText("hello");

    centralWidget()->show();
}
