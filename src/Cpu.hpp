#pragma once

#include <cstdint>
#include <string>

#include "Bus.hpp"

namespace smb
{
class Cpu
{
public:
    static const uint32_t kPrgCrc;
    static const uint16_t kResetVector;
    static const uint16_t kNmiVector;

    explicit Cpu(Bus& bus) : bus(bus) {}

    void reset();
    void nmi();

    bool hasFaulted() const { return faulted; }
    const std::string& faultMessage() const { return faultText; }

private:
    void execute(uint16_t entry);
    void fault(int code, uint16_t address);

    Bus& bus;
    uint8_t A = 0;
    uint8_t X = 0;
    uint8_t Y = 0;
    uint8_t S = 0xFD;
    bool C = false;
    bool Z = false;
    bool N = false;
    bool V = false;
    bool I = true;
    bool D = false;
    bool faulted = false;
    std::string faultText;
};
}
