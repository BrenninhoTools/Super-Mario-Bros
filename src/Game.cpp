#include "Game.hpp"

namespace smb
{
namespace
{
const double kFrameRate = 60.0988;
}

Game::Game() : bus(ppu, apu), cpu(bus), pixels(Ppu::kWidth * Ppu::kHeight, 0xFF000000u)
{
    apu.reset(sampleRate);
}

void Game::setSampleRate(int rate)
{
    sampleRate = rate;
    apu.reset(rate);
}

bool Game::loadRom(const std::string& path, std::string& error)
{
    if (!rom.loadFile(path, error))
    {
        return false;
    }
    attach();
    return true;
}

bool Game::loadRom(const uint8_t* data, size_t size, std::string& error)
{
    if (!rom.loadMemory(data, size, error))
    {
        return false;
    }
    attach();
    return true;
}

void Game::attach()
{
    bus.setPrg(rom.prg());
    ppu.setChr(rom.chr());
    reset();
}

void Game::reset()
{
    if (!rom.loaded())
    {
        return;
    }
    bus.clearRam();
    ppu.reset();
    apu.reset(sampleRate);
    sampleRemainder = 0.0;
    cpu.reset();
}

void Game::runFrame(uint8_t pad1, uint8_t pad2)
{
    if (!ready())
    {
        return;
    }
    bus.pads[0].setButtons(pad1);
    bus.pads[1].setButtons(pad2);
    ppu.beginFrame();
    if (ppu.nmiEnabled())
    {
        cpu.nmi();
    }
    ppu.render(pixels.data());
}

int Game::produceAudio(int16_t* out, int maxSamples, double rate)
{
    apu.setRateAdjust(rate);
    sampleRemainder += sampleRate * rate / kFrameRate;
    int count = (int)sampleRemainder;
    sampleRemainder -= count;
    if (count > maxSamples)
    {
        count = maxSamples;
    }
    apu.generate(out, count);
    return count;
}
}
