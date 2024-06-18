#include <cstring>
#include <algorithm>
#include <cassert>

#include "Util.h"
#include "TrackviewGUI.h"
#include "ColorDef.h"
#include "Debug.h"
#include "Xcept.h"

const std::vector<const char *> TrackviewGUI::noteNames = {
    "C-2", "C#-2", "D-2", "D#-2", "E-2", "F-2", "F#-2", "G-2", "G#-2", "A-2", "A#-2", "B-2",
    "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
    "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8"
};

/*
 * public
 */

TrackviewGUI::TrackviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
    : CursesWin(height, width, yPos, xPos) 
{
    // will clear the screen due to not overriding from CursesWin base class
    cursorPos = 0;
    maxChannels = 0;
    activeChannels = 0;
    cursorVisible = false;
    update();
}

TrackviewGUI::~TrackviewGUI() 
{
}

void TrackviewGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

void TrackviewGUI::SetState(const MP2KVisualizerState &songState)
{
    disp = songState;
    update();
}

void TrackviewGUI::SetTitle(const std::string& name)
{
    songName = name;
}

void TrackviewGUI::Enter() 
{
    this->cursorVisible = true;
    update();
}

void TrackviewGUI::Leave()
{
    this->cursorVisible = false;
    update();
}

void TrackviewGUI::PageDown()
{
    for (int i = 0; i < 5; i++) {
        scrollDownNoUpdate();
    }
    update();
}

void TrackviewGUI::PageUp()
{
    for (int i = 0; i < 5; i++) {
        scrollUpNoUpdate();
    }
    update();
}

void TrackviewGUI::ScrollDown()
{
    scrollDownNoUpdate();
    update();
}

void TrackviewGUI::ScrollUp()
{
    scrollUpNoUpdate();
    update();
}

/*
 * private Trackview
 */

void TrackviewGUI::update() 
{
    // init draw
    const uint32_t yBias = 1;
    const uint32_t xBias = 1;
    const size_t ntrks = disp.tracksUsed;
    // clear field
    if (cursorPos >= ntrks && cursorPos > 0) {
        cursorPos = (uint32_t)ntrks - 1;
    }
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    for (uint32_t i = yBias + 1 + (uint32_t)ntrks * 2; i < height; i++) {
        mvwhline(winPtr, (int)i, xBias, ' ', width - xBias);
    }
    // draw borderlines
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    mvwvline(winPtr, 1, 0, ' ', height - 1);
    mvwprintw(winPtr, 0, 0, "%-*s", width, " Tracker");

    // draw track titlebar
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_UNDERLINE);
    mvwprintw(winPtr, yBias, xBias, "[");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_NUM)) | A_UNDERLINE);
    wprintw(winPtr, "#T");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_UNDERLINE);
    wprintw(winPtr, "] ");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_LOC)) | A_UNDERLINE);
    wprintw(winPtr, "Location ");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_DEL)) | A_UNDERLINE);
    wprintw(winPtr, "Del");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_NOTE)) | A_UNDERLINE);
    wprintw(winPtr, "Note");
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_UNDERLINE);
    wprintw(winPtr, " - %-*s %3d/%3d", width - 24 - 3 - 8, songName.c_str(), activeChannels, maxChannels);

    for (uint32_t i = 0, th = 0; i < ntrks; i++, th += 2) {
        int aFlag = (cursorVisible && i == cursorPos) ? A_REVERSE : 0;
        const auto &trk = disp.tracks[i];

        // print tickbox and first line
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        mvwprintw(winPtr, (int)(yBias + 1 + th), xBias, "[");
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(disp.tracks[i].isMuted ? Color::TRK_NUM_MUTED : Color::TRK_NUM)) | aFlag);
        wprintw(winPtr, "%02d", i);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        wprintw(winPtr, "] ");
        wattrset(winPtr, (trk.isCalling ? COLOR_PAIR(static_cast<int>(Color::TRK_LOC_CALL)) : COLOR_PAIR(static_cast<int>(Color::TRK_LOC))));
        wprintw(winPtr, "0x%07X", trk.trackPtr);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_DEL)));
        wprintw(winPtr, "W%02d", trk.delay);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_NOTE)));

        // print notes
#define DECIDE_COL_C(a, b)\
        !a && !b ? static_cast<int>(Color::TRK_FGB_BGCW) :\
        !a &&  b ? static_cast<int>(Color::TRK_FGC_BGCW) :\
        a  && !b ? static_cast<int>(Color::TRK_FGB_BGC) :\
        static_cast<int>(Color::TRK_FGC_BGC)

#define DECIDE_COL_D(a, b)\
        !a && !b ? static_cast<int>(Color::TRK_FGB_BGW) :\
        !a &&  b ? static_cast<int>(Color::TRK_FGC_BGW) :\
        a  && !b ? static_cast<int>(Color::TRK_FGB_BGC) :\
        static_cast<int>(Color::TRK_FGC_BGC)

#define DECIDE_COL_E(a, b)\
        !a && !b ? static_cast<int>(Color::TRK_FGW_BGW) :\
        !a &&  b ? static_cast<int>(Color::TRK_FGEC_BGW) :\
        a  && !b ? static_cast<int>(Color::TRK_FGW_BGC) :\
        static_cast<int>(Color::TRK_FGEC_BGC)

        for (size_t j = 0;; j++) {
            // C and C#
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_C(
                            trk.activeNotes[j*12+0],
                            trk.activeNotes[j*12+1])));
            waddstr(winPtr, "\u259D");

            // D and D#
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_D(
                        trk.activeNotes[j*12+2],
                        trk.activeNotes[j*12+3])));
            waddstr(winPtr, "\u259D");

            // E and F
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_E(
                        trk.activeNotes[j*12+4],
                        trk.activeNotes[j*12+5])));
            waddstr(winPtr, "\u2590");

            // F# and G
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_D(
                        trk.activeNotes[j*12+7],
                        trk.activeNotes[j*12+6])));
            waddstr(winPtr, "\u2598");

            // keys only go up to key 127, this is the breakout point
            if (j == 10)
                break;

            // G# and A
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_D(
                        trk.activeNotes[j*12+9],
                        trk.activeNotes[j*12+8])));
            waddstr(winPtr, "\u2598");

            // A# and B
            wattrset(winPtr, COLOR_PAIR(DECIDE_COL_D(
                        trk.activeNotes[j*12+11],
                        trk.activeNotes[j*12+10])));
            waddstr(winPtr, "\u2598");
        }

#undef DECIDE_COL_C
#undef DECIDE_COL_D
#undef DECIDE_COL_E

        // FIXME
        //char noteBuffer[width + 1];
        //size_t noteBufferIndex = 0;

        //for (size_t j = 0; j < disp.data[i].activeNotes.size(); j++) {
            //if (disp.data[i].activeNotes[j]) {
                //CStrAppend(noteBuffer, &noteBufferIndex, noteNames[j]);
                //noteBuffer[noteBufferIndex++] = ' ';
            //}
        //}

        //if (width < 20) {
            //noteBuffer[0] = '\0';
        //} else if (noteBufferIndex > width - 20) {
            //noteBuffer[width - 20] = '\0';
        //} else {
            //size_t leftChars = width - 20 - noteBufferIndex;
            //for (size_t j = 0; j < leftChars; j++)
                //noteBuffer[noteBufferIndex++] = ' ';
            //noteBuffer[noteBufferIndex++] = '\0';
        //}

        //wprintw(winPtr, "%s", noteBuffer);
        // FIXME

        // print track values and sencond line
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        mvwprintw(winPtr, (int)(yBias + 2 + th), xBias, "    ");
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_VOICE)));
        if (trk.prog == PROG_UNDEFINED) {
            wprintw(winPtr, "---");
        } else {
            wprintw(winPtr, "%-3d", trk.prog);
        }
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_PAN)));
        wprintw(winPtr, " %-+3d", trk.pan);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_VOL)));
        wprintw(winPtr, " %-3d", trk.vol);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_MOD)));
        wprintw(winPtr, " %-3d", trk.mod);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_PITCH)));
        wprintw(winPtr, " %-+6d", trk.pitch);

        // print volume level
        static const char *fracBar[] = {
            " ", "\u258f", "\u258e", "\u258d", "\u258c", "\u258b", "\u258a", "\u2589", "\u2588"
        };

        auto printBar16 = [](char *buffer, int level) {
            assert(level <= 128 && level >= 0);
            for (size_t i = 0; i < 16; i++) {
                char ch;
                const char *part;
                if (level > 8) {
                    part = fracBar[8];
                    while ((ch = *part++) != '\0')
                        *buffer++ = ch;
                    level -= 8;
                } else if (level > 0) {
                    part = fracBar[level];
                    while ((ch = *part++) != '\0')
                        *buffer++ = ch;
                    level -= 8;
                } else {
                    part = fracBar[0];
                    while ((ch = *part++) != '\0')
                        *buffer++ = ch;
                }
            }
            *buffer = '\0';
            return;
        };

        // should be big enough for 16 unicode block bars
        char bar[16 * strlen("\u2688") + 1];
        uint32_t leftBar = trk.envL / 2;
        uint32_t rightBar = trk.envR / 2;

        printBar16(bar, 128 - leftBar);

        wattrset(winPtr, COLOR_PAIR(static_cast<int>(!trk.isMuted ? Color::TRK_LOUDNESS : Color::TRK_LOUDNESS_MUTED)) | A_REVERSE);
        wprintw(winPtr, "%s", bar);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::TRK_LOUDNESS)));
        wprintw(winPtr, "\u2503");

        printBar16(bar, rightBar);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(!trk.isMuted ? Color::TRK_LOUDNESS : Color::TRK_LOUDNESS_MUTED)));
        wprintw(winPtr, "%s", bar);
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
        whline(winPtr, ' ', width - 60);
    }

    wrefresh(winPtr);
}

void TrackviewGUI::scrollDownNoUpdate() 
{
    if (cursorPos + 1 < disp.tracksUsed)
        cursorPos++;
}

void TrackviewGUI::scrollUpNoUpdate()
{
    if (cursorPos > 0)
        cursorPos--;
}

void TrackviewGUI::ForceUpdate()
{
    update();
}
