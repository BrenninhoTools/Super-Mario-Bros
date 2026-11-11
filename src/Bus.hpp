#pragma once

#include <cstdint>

#include "Apu.hpp"
#include "Controller.hpp"
#include "Ppu.hpp"

namespace smb
{
class Bus
{
public:
    Bus(Ppu& ppu, Apu& apu) : ppu(ppu), apu(apu) {}

    void setPrg(const uint8_t* data) { prg = data; }
    void clearRam();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    uint8_t ram[0x800] = {};
    Controller pads[2];

private:
    Ppu& ppu;
    Apu& apu;
    const uint8_t* prg = nullptr;
};
}
