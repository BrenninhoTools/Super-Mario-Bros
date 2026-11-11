#pragma once

#include <SDL.h>

#include <cstdint>

namespace smb
{
class AudioOutput
{
public:
    bool open(int preferredRate, int bufferSamples, double latencyFrames);
    void close();
    void pause(bool paused);
    void flush();

    double rateAdjust();
    void submit(const int16_t* samples, int count);

    bool isOpen() const { return device != 0; }
    int sampleRate() const { return rate; }
    int queuedSamples() const;

private:
    SDL_AudioDeviceID device = 0;
    int rate = 48000;
    double targetSamples = 1600.0;
    double smoothedRate = 1.0;
    bool primed = false;
};
}
