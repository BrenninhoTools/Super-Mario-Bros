#pragma once

namespace smb
{
class FramePacer
{
public:
    static constexpr double kGameRate = 60.0988;

    void setRefreshRate(int hertz);
    int framesToRun(double elapsed, bool active);
    void reset();

    bool locked() const { return candidate && vsyncWorking; }
    int refreshRate() const { return refresh; }
    int interval() const { return vsyncInterval; }

private:
    int refresh = 60;
    bool configured = false;
    int vsyncInterval = 1;
    bool candidate = false;
    bool vsyncWorking = true;
    int presentCounter = 0;
    double accumulator = 0.0;
    double measureTime = 0.0;
    int measureCount = 0;
};
}
