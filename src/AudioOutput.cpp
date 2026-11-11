#include "AudioOutput.hpp"

#include <algorithm>
#include <vector>

#include "FramePacer.hpp"

namespace smb
{
namespace
{
const double kMaxRateDeviation = 0.005;
}

bool AudioOutput::open(int preferredRate, int bufferSamples, double latencyFrames)
{
    SDL_AudioSpec want = {};
    want.freq = preferredRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = (Uint16)bufferSamples;
    SDL_AudioSpec have = {};
    device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!device)
    {
        return false;
    }
    rate = have.freq;
    targetSamples = std::max((double)have.samples, rate / FramePacer::kGameRate * latencyFrames);
    smoothedRate = 1.0;
    primed = false;
    SDL_PauseAudioDevice(device, 0);
    return true;
}

void AudioOutput::close()
{
    if (device)
    {
        SDL_CloseAudioDevice(device);
        device = 0;
    }
}

void AudioOutput::pause(bool paused)
{
    if (!device)
    {
        return;
    }
    SDL_PauseAudioDevice(device, paused ? 1 : 0);
    if (paused)
    {
        flush();
    }
}

void AudioOutput::flush()
{
    if (device)
    {
        SDL_ClearQueuedAudio(device);
    }
    smoothedRate = 1.0;
    primed = false;
}

int AudioOutput::queuedSamples() const
{
    return device ? (int)(SDL_GetQueuedAudioSize(device) / sizeof(int16_t)) : 0;
}

double AudioOutput::rateAdjust()
{
    if (!device)
    {
        return 1.0;
    }
    double error = (queuedSamples() - targetSamples) / targetSamples;
    error = std::max(-1.0, std::min(1.0, error));
    double wanted = 1.0 - error * kMaxRateDeviation;
    smoothedRate += (wanted - smoothedRate) * 0.1;
    return smoothedRate;
}

void AudioOutput::submit(const int16_t* samples, int count)
{
    if (!device || count <= 0)
    {
        return;
    }
    int queued = queuedSamples();
    if (queued > targetSamples * 4)
    {
        SDL_ClearQueuedAudio(device);
        queued = 0;
        primed = false;
    }
    if (!primed || queued == 0)
    {
        std::vector<int16_t> silence((size_t)(targetSamples * 0.5), samples[0]);
        SDL_QueueAudio(device, silence.data(), (Uint32)(silence.size() * sizeof(int16_t)));
        primed = true;
    }
    SDL_QueueAudio(device, samples, (Uint32)count * sizeof(int16_t));
}
}
