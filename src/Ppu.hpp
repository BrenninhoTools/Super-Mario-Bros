#pragma once

#include <cstdint>

namespace smb
{
class Ppu
{
public:
    static constexpr int kWidth = 256;
    static constexpr int kHeight = 240;

    void reset();
    void setChr(const uint8_t* data);

    uint8_t readRegister(int reg);
    void writeRegister(int reg, uint8_t value);
    void dma(const uint8_t* page);

    void beginFrame();
    bool nmiEnabled() const { return (ctrl & 0x80) != 0; }
    void render(uint32_t* pixels);

    bool spriteLimit = true;

private:
    struct ScrollState
    {
        uint8_t ctrl = 0;
        uint8_t mask = 0;
        uint16_t x = 0;
        uint16_t y = 0;
    };

    uint8_t readVram(uint16_t address);
    void writeVram(uint16_t address, uint8_t value);
    uint16_t nametableIndex(uint16_t address) const;
    void renderBackground(const ScrollState& state, int line, uint8_t* out) const;
    void renderLine(int line, const ScrollState& state, uint32_t* row, bool& hitFound);

    const uint8_t* chr = nullptr;
    uint8_t nametables[0x800] = {};
    uint8_t palette[0x20] = {};
    uint8_t oam[0x100] = {};
    uint8_t oamAddress = 0;
    uint8_t ctrl = 0;
    uint8_t mask = 0;
    uint8_t scrollX = 0;
    uint8_t scrollY = 0;
    uint16_t vramAddress = 0;
    uint8_t readBuffer = 0;
    bool latch = false;
    uint8_t statusToggle = 0;
    bool splitRequested = false;
    ScrollState topState;
};
}
