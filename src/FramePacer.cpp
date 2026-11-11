#include "FramePacer.hpp"

#include <algorithm>
#include <cmath>

namespace smb
{
namespace
{
const int kMaxCatchUp = 4;
const int kMeasureWindow = 90;
}

void FramePacer::setRefreshRate(int hertz)
{
    if (hertz <= 0)
    {
        hertz = 60;
    }
    if (configured && hertz == refresh)
    {
        return;
    }
    configured = true;
    refresh = hertz;
    int multiple = (int)std::lround(refresh / kGameRate);
    candidate = false;
    vsyncInterval = 1;
    if (multiple >= 1)
    {
        double effective = (double)refresh / multiple;
        if (std::fabs(effective - kGameRate) / kGameRate < 0.025)
        {
            candidate = true;
            vsyncInterval = multiple;
        }
    }
    vsyncWorking = true;
    reset();
}

void FramePacer::reset()
{
    presentCounter = 0;
    accumulator = 0.0;
    measureTime = 0.0;
    measureCount = 0;
}

int FramePacer::framesToRun(double elapsed, bool active)
{
    if (elapsed < 0.0)
    {
        elapsed = 0.0;
    }
    if (elapsed > 0.25)
    {
        elapsed = 0.25;
    }

    if (candidate)
    {
        measureTime += elapsed;
        if (++measureCount >= kMeasureWindow)
        {
            double average = measureTime / measureCount;
            vsyncWorking = average > 0.7 / refresh;
            measureTime = 0.0;
            measureCount = 0;
        }
    }

    if (!active)
    {
        presentCounter = 0;
        accumulator = 0.0;
        return 0;
    }

    if (locked())
    {
        int vsyncs = (int)std::lround(elapsed * refresh);
        presentCounter += std::min(vsyncs, vsyncInterval * kMaxCatchUp);
        int frames = presentCounter / vsyncInterval;
        presentCounter %= vsyncInterval;
        return std::min(frames, kMaxCatchUp);
    }

    const double frameSeconds = 1.0 / kGameRate;
    accumulator += elapsed;
    int frames = 0;
    while (accumulator >= frameSeconds && frames < kMaxCatchUp)
    {
        accumulator -= frameSeconds;
        frames++;
    }
    if (accumulator > frameSeconds * kMaxCatchUp)
    {
        accumulator = 0.0;
    }
    return frames;
}
}
