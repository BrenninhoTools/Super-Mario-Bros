#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace smb
{
class Rom
{
public:
    bool loadFile(const std::string& path, std::string& error);
    bool loadMemory(const uint8_t* data, size_t size, std::string& error);

    const uint8_t* prg() const { return prgData.data(); }
    const uint8_t* chr() const { return chrData.data(); }
    bool loaded() const { return isLoaded; }

    static uint32_t crc32(const uint8_t* data, size_t size);

private:
    std::vector<uint8_t> prgData;
    std::vector<uint8_t> chrData;
    bool isLoaded = false;
};
}
