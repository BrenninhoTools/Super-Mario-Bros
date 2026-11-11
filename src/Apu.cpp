#include "Apu.hpp"

#include <cmath>

namespace smb
{
namespace
{
const double kCpuClock = 1789773.0;
const int32_t kSequencerPeriod = 7457;
const uint8_t kLengthTable[32] = {10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                                  12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};
const uint8_t kDutyTable[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 1, 1, 1, 0, 0, 0}, {1, 0, 0, 1, 1, 1, 1, 1}};
const uint8_t kTriangleTable[32] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const uint16_t kNoisePeriods[16] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};
}

void Apu::Envelope::clock()
{
    if (start)
    {
        start = false;
        decay = 15;
        divider = period;
        return;
    }
    if (divider == 0)
    {
        divider = period;
        if (decay > 0)
        {
            decay--;
        }
        else if (loop)
        {
            decay = 15;
        }
    }
    else
    {
        divider--;
    }
}

uint16_t Apu::Pulse::target() const
{
    uint16_t change = timer >> sweepShift;
    if (sweepNegate)
    {
        int value = (int)timer - (int)change - (secondChannel ? 0 : 1);
        return (uint16_t)(value < 0 ? 0 : value);
    }
    return (uint16_t)(timer + change);
}

bool Apu::Pulse::muted() const
{
    return timer < 8 || target() > 0x7FF;
}

void Apu::Pulse::advance(int cycles)
{
    int32_t period = (timer + 1) * 2;
    counter -= cycles;
    while (counter <= 0)
    {
        counter += period;
        step = (step + 1) & 7;
    }
}

void Apu::Pulse::clockSweep()
{
    if (sweepDivider == 0 && sweepEnabled && sweepShift > 0 && !muted())
    {
        timer = target();
    }
    if (sweepDivider == 0 || sweepReload)
    {
        sweepDivider = sweepPeriod;
        sweepReload = false;
    }
    else
    {
        sweepDivider--;
    }
}

uint8_t Apu::Pulse::output() const
{
    if (!enabled || length == 0 || muted() || !kDutyTable[duty][step])
    {
        return 0;
    }
    return envelope.output();
}

void Apu::Triangle::advance(int cycles)
{
    int32_t period = timer + 1;
    counter -= cycles;
    while (counter <= 0)
    {
        counter += period;
        if (length > 0 && linear > 0 && timer >= 2)
        {
            step = (step + 1) & 31;
        }
    }
}

uint8_t Apu::Triangle::output() const
{
    if (!enabled)
    {
        return 0;
    }
    return kTriangleTable[step];
}

void Apu::Noise::advance(int cycles)
{
    counter -= cycles;
    while (counter <= 0)
    {
        counter += period;
        uint16_t feedback = (shift & 1) ^ ((shift >> (mode ? 6 : 1)) & 1);
        shift = (uint16_t)((shift >> 1) | (feedback << 14));
    }
}

uint8_t Apu::Noise::output() const
{
    if (!enabled || length == 0 || (shift & 1))
    {
        return 0;
    }
    return envelope.output();
}

void Apu::reset(int sampleRate)
{
    pulse1 = Pulse();
    pulse2 = Pulse();
    pulse2.secondChannel = true;
    triangle = Triangle();
    noise = Noise();
    fiveStep = false;
    sequencerStep = 0;
    sequencerCounter = kSequencerPeriod;
    cyclesPerSample = kCpuClock / sampleRate;
    cycleRemainder = 0.0;
    highPassPrev = highPassOut = lowPassOut = 0.0f;
    const double pi = 3.14159265358979323846;
    double dt = 1.0 / sampleRate;
    double rcHigh = 1.0 / (2.0 * pi * 37.0);
    double rcLow = 1.0 / (2.0 * pi * 14000.0);
    highPassAlpha = (float)(rcHigh / (rcHigh + dt));
    lowPassAlpha = (float)(dt / (rcLow + dt));
}

void Apu::writeRegister(uint16_t address, uint8_t value)
{
    switch (address)
    {
    case 0x4000:
    case 0x4004:
    {
        Pulse& p = address == 0x4000 ? pulse1 : pulse2;
        p.duty = value >> 6;
        p.halt = value & 0x20;
        p.envelope.loop = value & 0x20;
        p.envelope.constant = value & 0x10;
        p.envelope.period = value & 0x0F;
        break;
    }
    case 0x4001:
    case 0x4005:
    {
        Pulse& p = address == 0x4001 ? pulse1 : pulse2;
        p.sweepEnabled = value & 0x80;
        p.sweepPeriod = (value >> 4) & 7;
        p.sweepNegate = value & 0x08;
        p.sweepShift = value & 7;
        p.sweepReload = true;
        break;
    }
    case 0x4002:
    case 0x4006:
    {
        Pulse& p = address == 0x4002 ? pulse1 : pulse2;
        p.timer = (uint16_t)((p.timer & 0x700) | value);
        break;
    }
    case 0x4003:
    case 0x4007:
    {
        Pulse& p = address == 0x4003 ? pulse1 : pulse2;
        p.timer = (uint16_t)((p.timer & 0xFF) | ((value & 7) << 8));
        if (p.enabled)
        {
            p.length = kLengthTable[value >> 3];
        }
        p.step = 0;
        p.envelope.start = true;
        break;
    }
    case 0x4008:
        triangle.control = value & 0x80;
        triangle.linearReload = value & 0x7F;
        break;
    case 0x400A:
        triangle.timer = (uint16_t)((triangle.timer & 0x700) | value);
        break;
    case 0x400B:
        triangle.timer = (uint16_t)((triangle.timer & 0xFF) | ((value & 7) << 8));
        if (triangle.enabled)
        {
            triangle.length = kLengthTable[value >> 3];
        }
        triangle.linearReloadFlag = true;
        break;
    case 0x400C:
        noise.halt = value & 0x20;
        noise.envelope.loop = value & 0x20;
        noise.envelope.constant = value & 0x10;
        noise.envelope.period = value & 0x0F;
        break;
    case 0x400E:
        noise.mode = value & 0x80;
        noise.period = kNoisePeriods[value & 0x0F];
        break;
    case 0x400F:
        if (noise.enabled)
        {
            noise.length = kLengthTable[value >> 3];
        }
        noise.envelope.start = true;
        break;
    case 0x4015:
        pulse1.enabled = value & 0x01;
        pulse2.enabled = value & 0x02;
        triangle.enabled = value & 0x04;
        noise.enabled = value & 0x08;
        if (!pulse1.enabled)
        {
            pulse1.length = 0;
        }
        if (!pulse2.enabled)
        {
            pulse2.length = 0;
        }
        if (!triangle.enabled)
        {
            triangle.length = 0;
        }
        if (!noise.enabled)
        {
            noise.length = 0;
        }
        break;
    case 0x4017:
        fiveStep = value & 0x80;
        sequencerStep = 0;
        sequencerCounter = kSequencerPeriod;
        if (fiveStep)
        {
            clockQuarter();
            clockHalf();
        }
        break;
    default:
        break;
    }
}

uint8_t Apu::readStatus() const
{
    return (uint8_t)((pulse1.length ? 0x01 : 0) | (pulse2.length ? 0x02 : 0) | (triangle.length ? 0x04 : 0) | (noise.length ? 0x08 : 0));
}

void Apu::clockQuarter()
{
    pulse1.envelope.clock();
    pulse2.envelope.clock();
    noise.envelope.clock();
    if (triangle.linearReloadFlag)
    {
        triangle.linear = triangle.linearReload;
    }
    else if (triangle.linear > 0)
    {
        triangle.linear--;
    }
    if (!triangle.control)
    {
        triangle.linearReloadFlag = false;
    }
}

void Apu::clockHalf()
{
    if (pulse1.length && !pulse1.halt)
    {
        pulse1.length--;
    }
    if (pulse2.length && !pulse2.halt)
    {
        pulse2.length--;
    }
    if (triangle.length && !triangle.control)
    {
        triangle.length--;
    }
    if (noise.length && !noise.halt)
    {
        noise.length--;
    }
    pulse1.clockSweep();
    pulse2.clockSweep();
}

void Apu::advanceSequencer(int cycles)
{
    sequencerCounter -= cycles;
    while (sequencerCounter <= 0)
    {
        sequencerCounter += kSequencerPeriod;
        if (fiveStep)
        {
            if (sequencerStep != 3)
            {
                clockQuarter();
            }
            if (sequencerStep == 1 || sequencerStep == 4)
            {
                clockHalf();
            }
            sequencerStep = (sequencerStep + 1) % 5;
        }
        else
        {
            clockQuarter();
            if (sequencerStep == 1 || sequencerStep == 3)
            {
                clockHalf();
            }
            sequencerStep = (sequencerStep + 1) % 4;
        }
    }
}

void Apu::generate(int16_t* out, int count)
{
    for (int i = 0; i < count; i++)
    {
        cycleRemainder += cyclesPerSample;
        int cycles = (int)cycleRemainder;
        cycleRemainder -= cycles;
        advanceSequencer(cycles);
        pulse1.advance(cycles);
        pulse2.advance(cycles);
        triangle.advance(cycles);
        noise.advance(cycles);

        float pulses = (float)(pulse1.output() + pulse2.output());
        float pulseOut = pulses > 0 ? 95.88f / (8128.0f / pulses + 100.0f) : 0.0f;
        float tnd = triangle.output() / 8227.0f + noise.output() / 12241.0f;
        float tndOut = tnd > 0 ? 159.79f / (1.0f / tnd + 100.0f) : 0.0f;
        float sample = pulseOut + tndOut;

        highPassOut = highPassAlpha * (highPassOut + sample - highPassPrev);
        highPassPrev = sample;
        lowPassOut += lowPassAlpha * (highPassOut - lowPassOut);

        float value = lowPassOut * 1.6f * volume;
        if (value > 1.0f)
        {
            value = 1.0f;
        }
        else if (value < -1.0f)
        {
            value = -1.0f;
        }
        out[i] = (int16_t)std::lround(value * 32767.0f);
    }
}
}
