#include "Bus.hpp"

#include <cstring>

namespace smb
{
void Bus::clearRam()
{
    std::memset(ram, 0, sizeof(ram));
}

uint8_t Bus::read(uint16_t address)
{
    if (address < 0x2000)
    {
        return ram[address & 0x7FF];
    }
    if (address < 0x4000)
    {
        return ppu.readRegister(address & 7);
    }
    if (address >= 0x8000)
    {
        return prg ? prg[address - 0x8000] : 0;
    }
    switch (address)
    {
    case 0x4015:
        return apu.readStatus();
    case 0x4016:
        return pads[0].read() | 0x40;
    case 0x4017:
        return pads[1].read() | 0x40;
    default:
        return 0;
    }
}

void Bus::write(uint16_t address, uint8_t value)
{
    if (address < 0x2000)
    {
        ram[address & 0x7FF] = value;
        return;
    }
    if (address < 0x4000)
    {
        ppu.writeRegister(address & 7, value);
        return;
    }
    if (address == 0x4014)
    {
        ppu.dma(&ram[(value << 8) & 0x700]);
        return;
    }
    if (address == 0x4016)
    {
        pads[0].strobe(value);
        pads[1].strobe(value);
        return;
    }
    if (address <= 0x4017)
    {
        apu.writeRegister(address, value);
    }
}
}
