#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace smb
{
struct ZipEntry
{
    std::string name;
    uint16_t method = 0;
    uint32_t crc = 0;
    uint32_t compressedSize = 0;
    uint32_t uncompressedSize = 0;
    uint32_t localOffset = 0;
    bool encrypted = false;
};

bool isZip(const uint8_t* data, size_t size);
bool listZip(const uint8_t* data, size_t size, std::vector<ZipEntry>& entries);
bool extractZipEntry(const uint8_t* data, size_t size, const ZipEntry& entry, std::vector<uint8_t>& out);
bool inflate(const uint8_t* data, size_t size, std::vector<uint8_t>& out, size_t maxOutput);
}
