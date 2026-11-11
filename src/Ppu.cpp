#include "Ppu.hpp"

#include <cstring>

namespace smb
{
namespace
{
const uint32_t kNesPalette[64] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
    0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
    0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
    0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
    0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
    0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000};
}

void Ppu::reset()
{
    std::memset(nametables, 0, sizeof(nametables));
    std::memset(palette, 0x0F, sizeof(palette));
    std::memset(oam, 0xF8, sizeof(oam));
    oamAddress = 0;
    ctrl = mask = 0;
    scrollX = scrollY = 0;
    vramAddress = 0;
    readBuffer = 0;
    latch = false;
    statusToggle = 0;
    splitRequested = false;
}

void Ppu::setChr(const uint8_t* data)
{
    chr = data;
}

uint16_t Ppu::nametableIndex(uint16_t address) const
{
    return address & 0x7FF;
}

uint8_t Ppu::readVram(uint16_t address)
{
    address &= 0x3FFF;
    if (address < 0x2000)
    {
        return chr ? chr[address] : 0;
    }
    if (address < 0x3F00)
    {
        return nametables[nametableIndex(address)];
    }
    uint8_t index = address & 0x1F;
    if ((index & 0x13) == 0x10)
    {
        index &= 0x0F;
    }
    return palette[index];
}

void Ppu::writeVram(uint16_t address, uint8_t value)
{
    address &= 0x3FFF;
    if (address < 0x2000)
    {
        return;
    }
    if (address < 0x3F00)
    {
        nametables[nametableIndex(address)] = value;
        return;
    }
    uint8_t index = address & 0x1F;
    if ((index & 0x13) == 0x10)
    {
        index &= 0x0F;
    }
    palette[index] = value & 0x3F;
}

uint8_t Ppu::readRegister(int reg)
{
    switch (reg)
    {
    case 2:
    {
        uint8_t value = statusToggle ? 0xC0 : 0x00;
        if (statusToggle)
        {
            topState.ctrl = ctrl;
            topState.mask = mask;
            topState.x = (uint16_t)(scrollX + ((ctrl & 0x01) ? 256 : 0));
            topState.y = (uint16_t)(scrollY + ((ctrl & 0x02) ? 240 : 0));
            splitRequested = true;
        }
        statusToggle ^= 1;
        latch = false;
        return value;
    }
    case 4:
        return oam[oamAddress];
    case 7:
    {
        uint16_t address = vramAddress & 0x3FFF;
        uint8_t value;
        if (address >= 0x3F00)
        {
            value = readVram(address);
            readBuffer = readVram(address - 0x1000);
        }
        else
        {
            value = readBuffer;
            readBuffer = readVram(address);
        }
        vramAddress += (ctrl & 0x04) ? 32 : 1;
        return value;
    }
    default:
        return 0;
    }
}

void Ppu::writeRegister(int reg, uint8_t value)
{
    switch (reg)
    {
    case 0:
        ctrl = value;
        break;
    case 1:
        mask = value;
        break;
    case 3:
        oamAddress = value;
        break;
    case 4:
        oam[oamAddress++] = value;
        break;
    case 5:
        if (latch)
        {
            scrollY = value;
        }
        else
        {
            scrollX = value;
        }
        latch = !latch;
        break;
    case 6:
        if (latch)
        {
            vramAddress = (uint16_t)((vramAddress & 0xFF00) | value);
        }
        else
        {
            vramAddress = (uint16_t)(((value & 0x3F) << 8) | (vramAddress & 0x00FF));
        }
        latch = !latch;
        break;
    case 7:
        writeVram(vramAddress, value);
        vramAddress += (ctrl & 0x04) ? 32 : 1;
        break;
    default:
        break;
    }
}

void Ppu::dma(const uint8_t* page)
{
    for (int i = 0; i < 256; i++)
    {
        oam[(uint8_t)(oamAddress + i)] = page[i];
    }
}

void Ppu::beginFrame()
{
    splitRequested = false;
}

uint8_t Ppu::backgroundPixel(const ScrollState& state, int x, int y) const
{
    int wx = (state.x + x) & 511;
    int wy = (state.y + y) % 480;
    int table = (wx >> 8) + (wy >= 240 ? 2 : 0);
    int py = wy % 240;
    int tx = (wx & 255) >> 3;
    int ty = py >> 3;
    uint16_t base = (uint16_t)(0x2000 + table * 0x400);
    uint8_t tile = nametables[nametableIndex((uint16_t)(base + ty * 32 + tx))];
    uint8_t attribute = nametables[nametableIndex((uint16_t)(base + 0x3C0 + (ty >> 2) * 8 + (tx >> 2)))];
    int shift = ((ty & 2) << 1) | (tx & 2);
    int group = (attribute >> shift) & 3;
    int patternBase = (state.ctrl & 0x10) ? 0x1000 : 0;
    int fy = py & 7;
    int fx = 7 - (wx & 7);
    uint8_t lo = chr[patternBase + tile * 16 + fy];
    uint8_t hi = chr[patternBase + tile * 16 + fy + 8];
    int color = ((lo >> fx) & 1) | (((hi >> fx) & 1) << 1);
    return color ? (uint8_t)((group << 2) | color) : 0;
}

void Ppu::renderLine(int line, const ScrollState& state, uint32_t* row, bool& hitFound)
{
    uint8_t background[kWidth];
    uint8_t sprite[kWidth];
    bool behind[kWidth];
    std::memset(background, 0, sizeof(background));
    std::memset(sprite, 0, sizeof(sprite));
    std::memset(behind, 0, sizeof(behind));

    bool showBackground = (state.mask & 0x08) && chr;
    bool showSprites = (state.mask & 0x10) && chr;
    if (showBackground)
    {
        int start = (state.mask & 0x02) ? 0 : 8;
        for (int x = start; x < kWidth; x++)
        {
            background[x] = backgroundPixel(state, x, line);
        }
    }

    if (showSprites)
    {
        int height = (state.ctrl & 0x20) ? 16 : 8;
        int found = 0;
        for (int i = 0; i < 64; i++)
        {
            const uint8_t* entry = &oam[i * 4];
            int top = entry[0] + 1;
            int row8 = line - top;
            if (row8 < 0 || row8 >= height)
            {
                continue;
            }
            if (spriteLimit && ++found > 8)
            {
                break;
            }
            uint8_t tile = entry[1];
            uint8_t attributes = entry[2];
            int sx = entry[3];
            if (attributes & 0x80)
            {
                row8 = height - 1 - row8;
            }
            int address;
            if (height == 16)
            {
                address = ((tile & 1) ? 0x1000 : 0) + (tile & 0xFE) * 16;
                if (row8 >= 8)
                {
                    address += 16;
                    row8 -= 8;
                }
            }
            else
            {
                address = ((state.ctrl & 0x08) ? 0x1000 : 0) + tile * 16;
            }
            uint8_t lo = chr[address + row8];
            uint8_t hi = chr[address + row8 + 8];
            int minX = (state.mask & 0x04) ? 0 : 8;
            for (int px = 0; px < 8; px++)
            {
                int x = sx + px;
                if (x >= kWidth)
                {
                    break;
                }
                if (x < minX)
                {
                    continue;
                }
                int bit = (attributes & 0x40) ? px : 7 - px;
                int color = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
                if (!color)
                {
                    continue;
                }
                if (i == 0 && background[x] && x != 255)
                {
                    hitFound = true;
                }
                if (sprite[x])
                {
                    continue;
                }
                sprite[x] = (uint8_t)(0x10 | ((attributes & 3) << 2) | color);
                behind[x] = attributes & 0x20;
            }
        }
    }

    uint8_t grayscale = (state.mask & 0x01) ? 0x30 : 0x3F;
    for (int x = 0; x < kWidth; x++)
    {
        uint8_t index;
        if (sprite[x] && (!behind[x] || !background[x]))
        {
            index = palette[sprite[x] & 0x1F];
        }
        else if (background[x])
        {
            index = palette[background[x]];
        }
        else
        {
            index = palette[0];
        }
        row[x] = 0xFF000000u | kNesPalette[index & grayscale & 0x3F];
    }
}

void Ppu::render(uint32_t* pixels)
{
    ScrollState bottom;
    bottom.ctrl = ctrl;
    bottom.mask = mask;
    bottom.x = (uint16_t)(scrollX + ((ctrl & 0x01) ? 256 : 0));
    bottom.y = (uint16_t)(scrollY + ((ctrl & 0x02) ? 240 : 0));
    bool split = splitRequested;
    bool switched = !split;
    bool hitFound = false;
    for (int line = 0; line < kHeight; line++)
    {
        const ScrollState& state = switched ? bottom : topState;
        renderLine(line, state, pixels + line * kWidth, hitFound);
        if (!switched && (hitFound || line >= 31))
        {
            switched = true;
        }
    }
}
}
