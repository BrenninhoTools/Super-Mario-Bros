#pragma once

#include <cstdint>

namespace smb
{
class Apu
{
public:
    void reset(int sampleRate);
    void writeRegister(uint16_t address, uint8_t value);
    uint8_t readStatus() const;
    void generate(int16_t* out, int count);

    float volume = 1.0f;

private:
    struct Envelope
    {
        bool start = false;
        bool loop = false;
        bool constant = false;
        uint8_t period = 0;
        uint8_t divider = 0;
        uint8_t decay = 0;

        void clock();
        uint8_t output() const { return constant ? period : decay; }
    };

    struct Pulse
    {
        bool enabled = false;
        bool secondChannel = false;
        uint8_t duty = 0;
        uint8_t step = 0;
        uint16_t timer = 0;
        int32_t counter = 0;
        uint8_t length = 0;
        bool halt = false;
        Envelope envelope;
        bool sweepEnabled = false;
        bool sweepNegate = false;
        bool sweepReload = false;
        uint8_t sweepPeriod = 0;
        uint8_t sweepShift = 0;
        uint8_t sweepDivider = 0;

        uint16_t target() const;
        bool muted() const;
        void advance(int cycles);
        void clockSweep();
        uint8_t output() const;
    };

    struct Triangle
    {
        bool enabled = false;
        uint8_t step = 0;
        uint16_t timer = 0;
        int32_t counter = 0;
        uint8_t length = 0;
        bool control = false;
        uint8_t linearReload = 0;
        uint8_t linear = 0;
        bool linearReloadFlag = false;

        void advance(int cycles);
        uint8_t output() const;
    };

    struct Noise
    {
        bool enabled = false;
        bool mode = false;
        uint16_t period = 4;
        int32_t counter = 0;
        uint16_t shift = 1;
        uint8_t length = 0;
        bool halt = false;
        Envelope envelope;

        void advance(int cycles);
        uint8_t output() const;
    };

    void clockQuarter();
    void clockHalf();
    void advanceSequencer(int cycles);

    Pulse pulse1;
    Pulse pulse2;
    Triangle triangle;
    Noise noise;
    bool fiveStep = false;
    int sequencerStep = 0;
    int32_t sequencerCounter = 0;
    double cyclesPerSample = 0.0;
    double cycleRemainder = 0.0;
    float highPassPrev = 0.0f;
    float highPassOut = 0.0f;
    float lowPassOut = 0.0f;
    float highPassAlpha = 0.0f;
    float lowPassAlpha = 0.0f;
};
}
