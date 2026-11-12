#include "Zip.hpp"

#include <cstring>

#include "Rom.hpp"

namespace smb
{
namespace
{
const uint16_t kLengthBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t kLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t kDistanceBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint8_t kDistanceExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const uint8_t kCodeLengthOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

uint16_t read16(const uint8_t* p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint32_t read32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

struct Huffman
{
    uint16_t counts[16];
    uint16_t symbols[288];

    bool build(const uint8_t* lengths, int count)
    {
        std::memset(counts, 0, sizeof(counts));
        for (int i = 0; i < count; i++)
        {
            counts[lengths[i]]++;
        }
        if (counts[0] == count)
        {
            return true;
        }
        int left = 1;
        for (int len = 1; len < 16; len++)
        {
            left <<= 1;
            left -= counts[len];
            if (left < 0)
            {
                return false;
            }
        }
        uint16_t offsets[16];
        offsets[1] = 0;
        for (int len = 1; len < 15; len++)
        {
            offsets[len + 1] = (uint16_t)(offsets[len] + counts[len]);
        }
        for (int i = 0; i < count; i++)
        {
            if (lengths[i])
            {
                symbols[offsets[lengths[i]]++] = (uint16_t)i;
            }
        }
        return true;
    }
};

class Inflater
{
public:
    Inflater(const uint8_t* data, size_t size, std::vector<uint8_t>& out, size_t maxOutput)
        : data(data), size(size), out(out), maxOutput(maxOutput)
    {
    }

    bool run()
    {
        bool last = false;
        while (!last)
        {
            last = bits(1) != 0;
            int type = (int)bits(2);
            bool ok;
            if (type == 0)
            {
                ok = stored();
            }
            else if (type == 1)
            {
                ok = fixed();
            }
            else if (type == 2)
            {
                ok = dynamic();
            }
            else
            {
                ok = false;
            }
            if (!ok || failed)
            {
                return false;
            }
        }
        return true;
    }

private:
    uint32_t bits(int count)
    {
        while (bitCount < count)
        {
            if (position >= size)
            {
                failed = true;
                return 0;
            }
            bitBuffer |= (uint32_t)data[position++] << bitCount;
            bitCount += 8;
        }
        uint32_t value = bitBuffer & ((1u << count) - 1u);
        bitBuffer >>= count;
        bitCount -= count;
        return value;
    }

    int decode(const Huffman& h)
    {
        int code = 0;
        int first = 0;
        int index = 0;
        for (int len = 1; len < 16; len++)
        {
            code |= (int)bits(1);
            if (failed)
            {
                return -1;
            }
            int count = h.counts[len];
            if (code - count < first)
            {
                return h.symbols[index + (code - first)];
            }
            index += count;
            first += count;
            first <<= 1;
            code <<= 1;
        }
        return -1;
    }

    bool stored()
    {
        bitBuffer = 0;
        bitCount = 0;
        if (position + 4 > size)
        {
            return false;
        }
        uint16_t length = read16(data + position);
        uint16_t inverse = read16(data + position + 2);
        position += 4;
        if ((uint16_t)~inverse != length || position + length > size || out.size() + length > maxOutput)
        {
            return false;
        }
        out.insert(out.end(), data + position, data + position + length);
        position += length;
        return true;
    }

    bool codes(const Huffman& lengthCodes, const Huffman& distanceCodes)
    {
        for (;;)
        {
            int symbol = decode(lengthCodes);
            if (symbol < 0)
            {
                return false;
            }
            if (symbol < 256)
            {
                if (out.size() >= maxOutput)
                {
                    return false;
                }
                out.push_back((uint8_t)symbol);
                continue;
            }
            if (symbol == 256)
            {
                return true;
            }
            symbol -= 257;
            if (symbol >= 29)
            {
                return false;
            }
            size_t length = kLengthBase[symbol] + bits(kLengthExtra[symbol]);
            int distanceSymbol = decode(distanceCodes);
            if (distanceSymbol < 0 || distanceSymbol >= 30)
            {
                return false;
            }
            size_t distance = kDistanceBase[distanceSymbol] + bits(kDistanceExtra[distanceSymbol]);
            if (failed || distance > out.size() || out.size() + length > maxOutput)
            {
                return false;
            }
            size_t from = out.size() - distance;
            for (size_t i = 0; i < length; i++)
            {
                out.push_back(out[from + i]);
            }
        }
    }

    bool fixed()
    {
        static Huffman lengthCodes;
        static Huffman distanceCodes;
        static bool ready = false;
        if (!ready)
        {
            uint8_t lengths[288];
            int i = 0;
            for (; i < 144; i++)
            {
                lengths[i] = 8;
            }
            for (; i < 256; i++)
            {
                lengths[i] = 9;
            }
            for (; i < 280; i++)
            {
                lengths[i] = 7;
            }
            for (; i < 288; i++)
            {
                lengths[i] = 8;
            }
            lengthCodes.build(lengths, 288);
            for (i = 0; i < 30; i++)
            {
                lengths[i] = 5;
            }
            distanceCodes.build(lengths, 30);
            ready = true;
        }
        return codes(lengthCodes, distanceCodes);
    }

    bool dynamic()
    {
        int literalCount = (int)bits(5) + 257;
        int distanceCount = (int)bits(5) + 1;
        int codeLengthCount = (int)bits(4) + 4;
        if (failed || literalCount > 286 || distanceCount > 30)
        {
            return false;
        }
        uint8_t lengths[320] = {};
        for (int i = 0; i < codeLengthCount; i++)
        {
            lengths[kCodeLengthOrder[i]] = (uint8_t)bits(3);
        }
        Huffman lengthCode;
        if (failed || !lengthCode.build(lengths, 19))
        {
            return false;
        }
        int index = 0;
        int total = literalCount + distanceCount;
        std::memset(lengths, 0, sizeof(lengths));
        while (index < total)
        {
            int symbol = decode(lengthCode);
            if (symbol < 0)
            {
                return false;
            }
            if (symbol < 16)
            {
                lengths[index++] = (uint8_t)symbol;
                continue;
            }
            uint8_t value = 0;
            int repeat;
            if (symbol == 16)
            {
                if (index == 0)
                {
                    return false;
                }
                value = lengths[index - 1];
                repeat = 3 + (int)bits(2);
            }
            else if (symbol == 17)
            {
                repeat = 3 + (int)bits(3);
            }
            else
            {
                repeat = 11 + (int)bits(7);
            }
            if (failed || index + repeat > total)
            {
                return false;
            }
            while (repeat--)
            {
                lengths[index++] = value;
            }
        }
        if (lengths[256] == 0)
        {
            return false;
        }
        Huffman lengthCodes;
        Huffman distanceCodes;
        if (!lengthCodes.build(lengths, literalCount) || !distanceCodes.build(lengths + literalCount, distanceCount))
        {
            return false;
        }
        return codes(lengthCodes, distanceCodes);
    }

    const uint8_t* data;
    size_t size;
    std::vector<uint8_t>& out;
    size_t maxOutput;
    size_t position = 0;
    uint32_t bitBuffer = 0;
    int bitCount = 0;
    bool failed = false;
};
}

bool inflate(const uint8_t* data, size_t size, std::vector<uint8_t>& out, size_t maxOutput)
{
    out.clear();
    Inflater inflater(data, size, out, maxOutput);
    return inflater.run();
}

bool isZip(const uint8_t* data, size_t size)
{
    return size >= 22 && data[0] == 'P' && data[1] == 'K' && (data[2] == 3 || data[2] == 5) && (data[3] == 4 || data[3] == 6);
}

bool listZip(const uint8_t* data, size_t size, std::vector<ZipEntry>& entries)
{
    entries.clear();
    if (size < 22)
    {
        return false;
    }
    size_t end = 0;
    bool found = false;
    size_t limit = size > 22 + 0xFFFF ? size - 22 - 0xFFFF : 0;
    for (size_t i = size - 22 + 1; i-- > limit;)
    {
        if (read32(data + i) == 0x06054B50u)
        {
            end = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        return false;
    }
    uint16_t count = read16(data + end + 10);
    uint32_t directoryOffset = read32(data + end + 16);
    size_t p = directoryOffset;
    for (uint16_t i = 0; i < count; i++)
    {
        if (p + 46 > size || read32(data + p) != 0x02014B50u)
        {
            return !entries.empty();
        }
        ZipEntry entry;
        uint16_t flags = read16(data + p + 8);
        entry.encrypted = (flags & 1) != 0;
        entry.method = read16(data + p + 10);
        entry.crc = read32(data + p + 16);
        entry.compressedSize = read32(data + p + 20);
        entry.uncompressedSize = read32(data + p + 24);
        uint16_t nameLength = read16(data + p + 28);
        uint16_t extraLength = read16(data + p + 30);
        uint16_t commentLength = read16(data + p + 32);
        entry.localOffset = read32(data + p + 42);
        if (p + 46 + nameLength > size)
        {
            return !entries.empty();
        }
        entry.name.assign((const char*)data + p + 46, nameLength);
        entries.push_back(entry);
        p += 46 + (size_t)nameLength + extraLength + commentLength;
    }
    return true;
}

bool extractZipEntry(const uint8_t* data, size_t size, const ZipEntry& entry, std::vector<uint8_t>& out)
{
    out.clear();
    if (entry.encrypted || (entry.method != 0 && entry.method != 8))
    {
        return false;
    }
    size_t local = entry.localOffset;
    if (local + 30 > size || read32(data + local) != 0x04034B50u)
    {
        return false;
    }
    size_t start = local + 30 + read16(data + local + 26) + read16(data + local + 28);
    if (start > size || entry.compressedSize > size - start)
    {
        return false;
    }
    const uint8_t* compressed = data + start;
    if (entry.method == 0)
    {
        if (entry.compressedSize != entry.uncompressedSize)
        {
            return false;
        }
        out.assign(compressed, compressed + entry.compressedSize);
    }
    else if (!inflate(compressed, entry.compressedSize, out, entry.uncompressedSize) || out.size() != entry.uncompressedSize)
    {
        out.clear();
        return false;
    }
    if (Rom::crc32(out.data(), out.size()) != entry.crc)
    {
        out.clear();
        return false;
    }
    return true;
}
}
