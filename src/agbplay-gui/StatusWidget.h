#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QPalette>

#include <array>
#include <bitset>

#include "VUBarWidget.h"

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget() override;

private:
    void paintEvent(QPaintEvent *paintEvent) override;

    const int WHITE_KEY_WIDTH = 6;
    const int BLACK_KEY_WIDTH = 5;
    const int OCTAVE_WIDTH = WHITE_KEY_WIDTH * 7; // 7 white keys per octave
    const int KEYBOARD_WIDTH = 10 * 7 * WHITE_KEY_WIDTH + 5 * WHITE_KEY_WIDTH + 1;
};

class SongWidget : public QWidget
{
    Q_OBJECT

public:
    SongWidget(QWidget *parent = nullptr);
    ~SongWidget() override;

private:
    void paintEvent(QPaintEvent *paintEvent) override;
};

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    TrackWidget(size_t trackNo, QWidget *parent = nullptr);
    ~TrackWidget() override;

private:
    QGridLayout layout{this};

    QLabel trackNoLabel{this};
    QPushButton muteButton{this};
    QPushButton soloButton{this};
    QPushButton harmonyButton{this};
    QLabel posLabel{this};
    QLabel restLabel{this};
    QLabel voiceTypeLabel{this};
    QLabel instNoLabel{this};
    QLabel volLabel{this};
    QLabel panLabel{this};
    QLabel modLabel{this};
    QLabel pitchLabel{this};
    KeyboardWidget keyboardWidget{this};

    QWidget vuBarWidget{this};
    QHBoxLayout vuBarLayout{&vuBarWidget};
    VUBarWidget vuBarWidgetLeft{VUBarWidget::Orientation::Left, false, -36.0f, 3.0f, &vuBarWidget};
    VUBarWidget vuBarWidgetRight{VUBarWidget::Orientation::Right, false, -36.0f, 3.0f, &vuBarWidget};
    QVBoxLayout vuBarKeyboardLayout;

    static const QPalette labelUnmutedPalette;
    static const QPalette labelMutedPalette;

    static const QPalette posPalette;
    static const QPalette restPalette;
    static const QPalette voiceTypePalette;
    static const QPalette instNoPalette;
    static const QPalette volPalette;
    static const QPalette panPalette;
    static const QPalette modPalette;
    static const QPalette pitchPalette;
    static const QFont labelFont;
    static const QFont labelMonospaceFont;
};

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    StatusWidget(QWidget *parent = nullptr);
    ~StatusWidget() override;

private:

    QVBoxLayout layout{this};

    SongWidget songWidget{this};
    // TODO use constant MAX_TRACKS instead of 16
    std::vector<TrackWidget *> trackWidgets;
};
