#include "Rom.hpp"

#include <SDL.h>

#include <cstdio>
#include <cstring>

#include "Cpu.hpp"

namespace smb
{
namespace
{
const size_t kPrgSize = 0x8000;
const size_t kChrSize = 0x2000;
}

uint32_t Rom::crc32(const uint8_t* data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
        {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

bool Rom::loadFile(const std::string& path, std::string& error)
{
    SDL_RWops* file = SDL_RWFromFile(path.c_str(), "rb");
    if (!file)
    {
        error = "Could not open " + path;
        return false;
    }
    Sint64 size = SDL_RWsize(file);
    if (size <= 0 || size > 4 * 1024 * 1024)
    {
        SDL_RWclose(file);
        error = "Invalid ROM file size";
        return false;
    }
    std::vector<uint8_t> data((size_t)size);
    size_t read = SDL_RWread(file, data.data(), 1, data.size());
    SDL_RWclose(file);
    if (read != data.size())
    {
        error = "Could not read " + path;
        return false;
    }
    return loadMemory(data.data(), data.size(), error);
}

bool Rom::loadMemory(const uint8_t* data, size_t size, std::string& error)
{
    isLoaded = false;
    size_t offset = 0;
    if (size >= 16 && std::memcmp(data, "NES\x1A", 4) == 0)
    {
        uint8_t prgBanks = data[4];
        uint8_t chrBanks = data[5];
        bool cleanHeader = data[12] == 0 && data[13] == 0 && data[14] == 0 && data[15] == 0;
        int mapper = (data[6] >> 4) | (cleanHeader ? (data[7] & 0xF0) : 0);
        if (prgBanks != 2 || chrBanks != 1 || mapper != 0)
        {
            error = "This is not a Super Mario Bros. ROM (wrong mapper or size)";
            return false;
        }
        offset = 16 + ((data[6] & 0x04) ? 512 : 0);
    }
    if (size < offset + kPrgSize + kChrSize)
    {
        error = "The ROM file is too small";
        return false;
    }
    uint32_t crc = crc32(data + offset, kPrgSize);
    if (crc != Cpu::kPrgCrc)
    {
        char text[160];
        std::snprintf(text, sizeof(text), "Unsupported ROM (PRG CRC %08X). Use Super Mario Bros. (World)", crc);
        error = text;
        return false;
    }
    prgData.assign(data + offset, data + offset + kPrgSize);
    chrData.assign(data + offset + kPrgSize, data + offset + kPrgSize + kChrSize);
    isLoaded = true;
    return true;
}
}
