#include "Rom.hpp"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "Cpu.hpp"
#include "Zip.hpp"

namespace smb
{
namespace
{
const size_t kPrgSize = 0x8000;
const size_t kChrSize = 0x2000;
const uint32_t kSmbChrCrc = 0x406061CDu;
const uint8_t kOriginalReset[8] = {0xD8, 0xA9, 0x10, 0x8D, 0x00, 0x20, 0xA2, 0xFF};

const size_t kMaxRomSize = 4 * 1024 * 1024;

bool hasExtension(const std::string& name, const char* extension)
{
    size_t length = std::strlen(extension);
    if (name.size() < length)
    {
        return false;
    }
    for (size_t i = 0; i < length; i++)
    {
        if (std::tolower((unsigned char)name[name.size() - length + i]) != extension[i])
        {
            return false;
        }
    }
    return true;
}

bool isMultiCartStub(const uint8_t* reset)
{
    return reset[0] == 0xA9 && reset[2] == 0x8D && reset[5] == 0x4C && reset[6] == 0x00 && reset[7] == 0x80;
}
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
    if (size <= 0 || (size_t)size > kMaxFileSize)
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
    if (isZip(data, size))
    {
        return loadZip(data, size, error);
    }
    return loadNes(data, size, error);
}

bool Rom::loadZip(const uint8_t* data, size_t size, std::string& error)
{
    std::vector<ZipEntry> entries;
    if (!listZip(data, size, entries))
    {
        error = "The ZIP file is damaged or empty";
        return false;
    }
    std::vector<const ZipEntry*> candidates;
    for (const ZipEntry& entry : entries)
    {
        bool folder = !entry.name.empty() && entry.name.back() == '/';
        bool metadata = entry.name.compare(0, 9, "__MACOSX/") == 0;
        if (folder || metadata || entry.uncompressedSize < kPrgSize + kChrSize || entry.uncompressedSize > kMaxRomSize)
        {
            continue;
        }
        candidates.push_back(&entry);
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const ZipEntry* a, const ZipEntry* b) {
        return hasExtension(a->name, ".nes") && !hasExtension(b->name, ".nes");
    });

    std::string lastError;
    bool sawEncrypted = false;
    std::vector<uint8_t> content;
    for (const ZipEntry* entry : candidates)
    {
        if (entry->encrypted)
        {
            sawEncrypted = true;
            continue;
        }
        if (!extractZipEntry(data, size, *entry, content))
        {
            if (hasExtension(entry->name, ".nes"))
            {
                lastError = "Could not extract " + entry->name + " from the ZIP file";
            }
            continue;
        }
        std::string entryError;
        if (loadNes(content.data(), content.size(), entryError))
        {
            return true;
        }
        if (hasExtension(entry->name, ".nes"))
        {
            lastError = entryError;
        }
    }
    if (!lastError.empty())
    {
        error = lastError;
    }
    else if (sawEncrypted)
    {
        error = "The ZIP file is password protected";
    }
    else
    {
        error = "No Super Mario Bros. ROM was found in the ZIP file";
    }
    return false;
}

bool Rom::loadNes(const uint8_t* data, size_t size, std::string& error)
{
    isLoaded = false;
    size_t prgOffset = 0;
    size_t prgBanks = 1;
    size_t chrBanks = 1;
    int mapper = 0;
    if (size >= 16 && std::memcmp(data, "NES\x1A", 4) == 0)
    {
        bool cleanHeader = data[12] == 0 && data[13] == 0 && data[14] == 0 && data[15] == 0;
        mapper = (data[6] >> 4) | (cleanHeader ? (data[7] & 0xF0) : 0);
        prgBanks = data[4] / 2;
        chrBanks = data[5];
        prgOffset = 16 + ((data[6] & 0x04) ? 512 : 0);
        bool supported = (mapper == 0 && data[4] == 2 && chrBanks == 1) ||
                         (mapper == 66 && prgBanks >= 1 && chrBanks >= 1 && (data[4] & 1) == 0);
        if (!supported)
        {
            error = "This is not a Super Mario Bros. ROM (wrong mapper or size)";
            return false;
        }
    }
    size_t chrOffset = prgOffset + prgBanks * kPrgSize;
    if (size < chrOffset + chrBanks * kChrSize)
    {
        error = "The ROM file is too small";
        return false;
    }

    std::vector<uint8_t> bank;
    size_t found = prgBanks;
    uint32_t firstCrc = 0;
    for (size_t i = 0; i < prgBanks; i++)
    {
        bank.assign(data + prgOffset + i * kPrgSize, data + prgOffset + (i + 1) * kPrgSize);
        if (isMultiCartStub(&bank[1]))
        {
            std::memcpy(&bank[1], kOriginalReset, sizeof(kOriginalReset));
        }
        uint32_t crc = crc32(bank.data(), bank.size());
        if (i == 0)
        {
            firstCrc = crc;
        }
        if (crc == Cpu::kPrgCrc)
        {
            found = i;
            break;
        }
    }
    if (found == prgBanks)
    {
        char text[160];
        std::snprintf(text, sizeof(text), "Unsupported ROM (PRG CRC %08X). Use Super Mario Bros. (World) or Super Mario Bros. / Duck Hunt", firstCrc);
        error = text;
        return false;
    }

    size_t chrBank = found < chrBanks ? found : chrBanks - 1;
    if (chrBanks > 1)
    {
        for (size_t i = 0; i < chrBanks; i++)
        {
            if (crc32(data + chrOffset + i * kChrSize, kChrSize) == kSmbChrCrc)
            {
                chrBank = i;
                break;
            }
        }
    }

    prgData = bank;
    chrData.assign(data + chrOffset + chrBank * kChrSize, data + chrOffset + (chrBank + 1) * kChrSize);
    isLoaded = true;
    return true;
}
}
