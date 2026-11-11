#pragma once

#include <SDL.h>

#include <cstdint>
#include <map>

namespace smb
{
class TouchControls
{
public:
    void layout(int width, int height);
    void handleEvent(const SDL_Event& event, int outputWidth, int outputHeight);
    uint8_t buttons() const;
    void draw(SDL_Renderer* renderer) const;
    bool hasTouches() const { return !fingers.empty(); }
    bool tapped();

    bool visible = false;

private:
    struct Circle
    {
        float x = 0;
        float y = 0;
        float r = 0;
    };

    struct Pill
    {
        float x = 0;
        float y = 0;
        float w = 0;
        float h = 0;
    };

    uint8_t dpadAt(float x, float y) const;
    uint8_t buttonsAt(float x, float y) const;

    std::map<SDL_FingerID, SDL_FPoint> fingers;
    Circle dpad;
    Circle buttonA;
    Circle buttonB;
    Pill buttonSelect;
    Pill buttonStart;
    float unit = 0;
    bool pendingTap = false;
};
}
