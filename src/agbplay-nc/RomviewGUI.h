#pragma once

#include <cstdint>
#include <string>
#include "CursesWin.h"
#include "Rom.h"
#include "SoundData.h"

class RomviewGUI : public CursesWin {
public:
    RomviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, const SongTableInfo &songTableInfo);
    ~RomviewGUI() override;

    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
private:
    void update() override;
    std::string gameName;
    std::string gameCode;
    const SongTableInfo &songTableInfo;
};
