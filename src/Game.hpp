#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Apu.hpp"
#include "Bus.hpp"
#include "Cpu.hpp"
#include "Ppu.hpp"
#include "Rom.hpp"

namespace smb
{
class Game
{
public:
    Game();

    bool loadRom(const std::string& path, std::string& error);
    bool loadRom(const uint8_t* data, size_t size, std::string& error);
    void reset();
    void runFrame(uint8_t pad1, uint8_t pad2);
    int produceAudio(int16_t* out, int maxSamples);

    void setSampleRate(int rate);
    const uint32_t* framebuffer() const { return pixels.data(); }
    bool ready() const { return rom.loaded() && !cpu.hasFaulted(); }
    bool faulted() const { return cpu.hasFaulted(); }
    const std::string& faultMessage() const { return cpu.faultMessage(); }

    Ppu& video() { return ppu; }
    Apu& audio() { return apu; }

private:
    void attach();

    Rom rom;
    Ppu ppu;
    Apu apu;
    Bus bus;
    Cpu cpu;
    std::vector<uint32_t> pixels;
    int sampleRate = 48000;
    double sampleRemainder = 0.0;
};
}
