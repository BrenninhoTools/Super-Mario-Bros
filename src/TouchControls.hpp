#pragma once

#include <SDL.h>

#include <cstdint>
#include <map>

namespace smb
{
class TouchControls
{
public:
    void layout(int width, int height, float insetLeft = 0.0f, float insetRight = 0.0f, float insetBottom = 0.0f);
    void handleEvent(const SDL_Event& event, int outputWidth, int outputHeight);
    uint8_t buttons() const;
    void draw(SDL_Renderer* renderer) const;
    bool hasTouches() const { return !fingers.empty(); }
    bool tapped();
    bool running() const { return runLatched; }
    void setRunning(bool value) { runLatched = value; }

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

    uint8_t stickButtons() const;
    SDL_FPoint knobOffset() const;
    uint8_t buttonsAt(float x, float y) const;
    bool inCircle(const Circle& c, float x, float y, float scale) const;
    bool inPill(const Pill& p, float x, float y) const;
    void drawStick(SDL_Renderer* renderer, uint8_t pressed) const;
    void drawRoundButton(SDL_Renderer* renderer, const Circle& c, const char* label, bool pressed, bool latched) const;
    void drawPill(SDL_Renderer* renderer, const Pill& p, const char* label, bool pressed) const;

    std::map<SDL_FingerID, SDL_FPoint> fingers;
    Circle stick;
    SDL_FingerID stickFinger = 0;
    bool stickActive = false;
    SDL_FPoint stickPoint = {0, 0};
    Circle buttonA;
    Circle buttonB;
    Pill buttonSelect;
    Pill buttonStart;
    float unit = 0;
    bool pendingTap = false;
    bool runLatched = false;
};
}
