#include "Ppu.hpp"

#include <cstring>

namespace smb
{
namespace
{
struct PatternTables
{
    uint64_t low[256];
    uint64_t high[256];

    PatternTables()
    {
        for (int value = 0; value < 256; value++)
        {
            uint8_t lo[8];
            uint8_t hi[8];
            for (int p = 0; p < 8; p++)
            {
                int bit = (value >> (7 - p)) & 1;
                lo[p] = (uint8_t)bit;
                hi[p] = (uint8_t)(bit << 1);
            }
            std::memcpy(&low[value], lo, 8);
            std::memcpy(&high[value], hi, 8);
        }
    }
};

const PatternTables kPatternTables;

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

void Ppu::renderBackground(const ScrollState& state, int line, uint8_t* out) const
{
    int wy = (state.y + line) % 480;
    int tableRow = wy >= 240 ? 2 : 0;
    int py = wy % 240;
    int ty = py >> 3;
    int fy = py & 7;
    int patternBase = (state.ctrl & 0x10) ? 0x1000 : 0;
    int wx = state.x & 511;
    int tileX = wx >> 3;
    int attributeShift = (ty & 2) << 1;
    int rowOffset = ty * 32;
    int attributeRow = 0x3C0 + (ty >> 2) * 8;
    uint8_t buffer[kWidth + 16];
    for (int t = 0; t < 33; t++)
    {
        int table = tableRow + (tileX >> 5);
        int tx = tileX & 31;
        int base = table * 0x400;
        uint8_t tile = nametables[(base + rowOffset + tx) & 0x7FF];
        uint8_t attribute = nametables[(base + attributeRow + (tx >> 2)) & 0x7FF];
        uint64_t group = (uint64_t)(((attribute >> (attributeShift | (tx & 2))) & 3) << 2);
        const uint8_t* pattern = chr + patternBase + tile * 16 + fy;
        uint64_t pixels = kPatternTables.low[pattern[0]] | kPatternTables.high[pattern[8]];
        uint64_t opaque = (pixels | (pixels >> 1)) & 0x0101010101010101ull;
        pixels |= opaque * group;
        std::memcpy(buffer + t * 8, &pixels, 8);
        tileX = (tileX + 1) & 63;
    }
    std::memcpy(out, buffer + (wx & 7), kWidth);
}

void Ppu::renderLine(int line, const ScrollState& state, uint32_t* row, bool& hitFound)
{
    uint8_t background[kWidth];
    uint8_t sprite[kWidth];
    std::memset(background, 0, sizeof(background));

    bool showBackground = (state.mask & 0x08) && chr;
    bool showSprites = (state.mask & 0x10) && chr;
    if (showBackground)
    {
        renderBackground(state, line, background);
        if (!(state.mask & 0x02))
        {
            std::memset(background, 0, 8);
        }
    }

    uint32_t colors[32];
    uint8_t grayscale = (state.mask & 0x01) ? 0x30 : 0x3F;
    for (int i = 0; i < 32; i++)
    {
        colors[i] = 0xFF000000u | kNesPalette[palette[i] & grayscale];
    }
    for (int i = 0x10; i < 0x20; i += 4)
    {
        colors[i] = colors[i - 0x10];
    }

    bool anySprite = false;
    if (showSprites)
    {
        int height = (state.ctrl & 0x20) ? 16 : 8;
        int found = 0;
        int minX = (state.mask & 0x04) ? 0 : 8;
        for (int i = 0; i < 64; i++)
        {
            const uint8_t* entry = &oam[i * 4];
            int rowInSprite = line - (entry[0] + 1);
            if (rowInSprite < 0 || rowInSprite >= height)
            {
                continue;
            }
            if (spriteLimit && ++found > 8)
            {
                break;
            }
            if (!anySprite)
            {
                std::memset(sprite, 0, sizeof(sprite));
                anySprite = true;
            }
            uint8_t tile = entry[1];
            uint8_t attributes = entry[2];
            int sx = entry[3];
            if (attributes & 0x80)
            {
                rowInSprite = height - 1 - rowInSprite;
            }
            int address;
            if (height == 16)
            {
                address = ((tile & 1) ? 0x1000 : 0) + (tile & 0xFE) * 16;
                if (rowInSprite >= 8)
                {
                    address += 16;
                    rowInSprite -= 8;
                }
            }
            else
            {
                address = ((state.ctrl & 0x08) ? 0x1000 : 0) + tile * 16;
            }
            uint8_t lo = chr[address + rowInSprite];
            uint8_t hi = chr[address + rowInSprite + 8];
            if (!(lo | hi))
            {
                continue;
            }
            uint8_t paletteBits = (uint8_t)(0x10 | ((attributes & 3) << 2));
            uint8_t behind = (attributes & 0x20) ? 0x80 : 0;
            bool flip = attributes & 0x40;
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
                int bit = flip ? px : 7 - px;
                int color = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
                if (!color)
                {
                    continue;
                }
                if (i == 0 && background[x] && x != 255)
                {
                    hitFound = true;
                }
                if (!sprite[x])
                {
                    sprite[x] = (uint8_t)(behind | paletteBits | color);
                }
            }
        }
    }

    uint32_t backdrop = colors[0];
    if (!anySprite)
    {
        for (int x = 0; x < kWidth; x++)
        {
            row[x] = background[x] ? colors[background[x]] : backdrop;
        }
        return;
    }
    for (int x = 0; x < kWidth; x++)
    {
        uint8_t s = sprite[x];
        uint8_t b = background[x];
        if (s && (!(s & 0x80) || !b))
        {
            row[x] = colors[s & 0x1F];
        }
        else
        {
            row[x] = b ? colors[b] : backdrop;
        }
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
