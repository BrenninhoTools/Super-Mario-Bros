#include "Cpu.hpp"

#include <cstdio>

namespace smb
{
void Cpu::reset()
{
    A = X = Y = 0;
    S = 0xFD;
    C = Z = N = V = D = false;
    I = true;
    faulted = false;
    faultText.clear();
    execute(kResetVector);
}

void Cpu::nmi()
{
    if (faulted)
    {
        return;
    }
    uint8_t* ram = bus.ram;
    ram[0x100 + S--] = 0x80;
    ram[0x100 + S--] = 0x57;
    ram[0x100 + S--] = (uint8_t)((N ? 0x80 : 0) | (V ? 0x40 : 0) | 0x20 | (D ? 0x08 : 0) | (I ? 0x04 : 0) | (Z ? 0x02 : 0) | (C ? 0x01 : 0));
    I = true;
    execute(kNmiVector);
}

void Cpu::fault(int code, uint16_t address)
{
    static const char* kinds[] = {"invalid opcode", "invalid entry point", "invalid return address", "invalid indirect jump"};
    char text[96];
    std::snprintf(text, sizeof(text), "CPU fault: %s at $%04X", kinds[code & 3], address);
    faulted = true;
    faultText = text;
}
}
