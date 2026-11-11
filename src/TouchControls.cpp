#include "TouchControls.hpp"

#include <algorithm>
#include <cmath>

#include "Controller.hpp"
#include "Draw.hpp"

namespace smb
{
void TouchControls::layout(int width, int height)
{
    unit = (float)std::min(width, height);
    float margin = unit * 0.06f;
    dpad.r = unit * 0.17f;
    dpad.x = margin + dpad.r;
    dpad.y = height - margin - dpad.r;
    buttonA.r = unit * 0.085f;
    buttonA.x = width - margin - buttonA.r;
    buttonA.y = height - margin - buttonA.r * 2.4f;
    buttonB.r = buttonA.r;
    buttonB.x = buttonA.x - buttonA.r * 2.5f;
    buttonB.y = height - margin - buttonB.r;
    buttonSelect.w = unit * 0.17f;
    buttonSelect.h = unit * 0.065f;
    buttonStart = buttonSelect;
    buttonSelect.x = width * 0.5f - buttonSelect.w - unit * 0.03f;
    buttonStart.x = width * 0.5f + unit * 0.03f;
    buttonSelect.y = buttonStart.y = height - margin * 0.6f - buttonSelect.h;
}

void TouchControls::handleEvent(const SDL_Event& event, int outputWidth, int outputHeight)
{
    switch (event.type)
    {
    case SDL_FINGERDOWN:
        pendingTap = true;
        fingers[event.tfinger.fingerId] = {event.tfinger.x * outputWidth, event.tfinger.y * outputHeight};
        break;
    case SDL_FINGERMOTION:
        fingers[event.tfinger.fingerId] = {event.tfinger.x * outputWidth, event.tfinger.y * outputHeight};
        break;
    case SDL_FINGERUP:
        fingers.erase(event.tfinger.fingerId);
        break;
    default:
        break;
    }
}

bool TouchControls::tapped()
{
    bool result = pendingTap;
    pendingTap = false;
    return result;
}

uint8_t TouchControls::dpadAt(float x, float y) const
{
    float dx = x - dpad.x;
    float dy = y - dpad.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance > dpad.r * 1.5f || distance < dpad.r * 0.12f)
    {
        return 0;
    }
    uint8_t result = 0;
    if (std::fabs(dx) > distance * 0.38f)
    {
        result |= dx > 0 ? ButtonRight : ButtonLeft;
    }
    if (std::fabs(dy) > distance * 0.38f)
    {
        result |= dy > 0 ? ButtonDown : ButtonUp;
    }
    return result;
}

uint8_t TouchControls::buttonsAt(float x, float y) const
{
    uint8_t result = 0;
    auto inCircle = [&](const Circle& c, float scale) {
        float dx = x - c.x;
        float dy = y - c.y;
        return dx * dx + dy * dy <= c.r * c.r * scale * scale;
    };
    auto inPill = [&](const Pill& p) {
        float pad = unit * 0.02f;
        return x >= p.x - pad && x <= p.x + p.w + pad && y >= p.y - pad && y <= p.y + p.h + pad;
    };
    bool a = inCircle(buttonA, 1.35f);
    bool b = inCircle(buttonB, 1.35f);
    if (!a && !b)
    {
        float mx = (buttonA.x + buttonB.x) * 0.5f;
        float my = (buttonA.y + buttonB.y) * 0.5f;
        float dx = x - mx;
        float dy = y - my;
        if (dx * dx + dy * dy <= buttonA.r * buttonA.r * 0.6f)
        {
            a = b = true;
        }
    }
    if (a)
    {
        result |= ButtonA;
    }
    if (b)
    {
        result |= ButtonB;
    }
    if (inPill(buttonSelect))
    {
        result |= ButtonSelect;
    }
    if (inPill(buttonStart))
    {
        result |= ButtonStart;
    }
    return result;
}

uint8_t TouchControls::buttons() const
{
    uint8_t result = 0;
    for (const auto& finger : fingers)
    {
        result |= dpadAt(finger.second.x, finger.second.y);
        result |= buttonsAt(finger.second.x, finger.second.y);
    }
    return result;
}

void TouchControls::draw(SDL_Renderer* renderer) const
{
    if (!visible)
    {
        return;
    }
    uint8_t pressed = buttons();
    SDL_Color base = {255, 255, 255, 60};
    SDL_Color active = {255, 255, 255, 140};
    SDL_Color label = {255, 255, 255, 190};
    SDL_Color highlight = {255, 214, 80, 255};

    fillCircle(renderer, dpad.x, dpad.y, dpad.r, base);
    float arm = dpad.r * 0.55f;
    float tip = dpad.r * 0.85f;
    float half = dpad.r * 0.22f;
    SDL_Color up = (pressed & ButtonUp) ? highlight : label;
    SDL_Color down = (pressed & ButtonDown) ? highlight : label;
    SDL_Color left = (pressed & ButtonLeft) ? highlight : label;
    SDL_Color right = (pressed & ButtonRight) ? highlight : label;
    fillTriangle(renderer, {dpad.x, dpad.y - tip}, {dpad.x - half, dpad.y - arm}, {dpad.x + half, dpad.y - arm}, up);
    fillTriangle(renderer, {dpad.x, dpad.y + tip}, {dpad.x + half, dpad.y + arm}, {dpad.x - half, dpad.y + arm}, down);
    fillTriangle(renderer, {dpad.x - tip, dpad.y}, {dpad.x - arm, dpad.y + half}, {dpad.x - arm, dpad.y - half}, left);
    fillTriangle(renderer, {dpad.x + tip, dpad.y}, {dpad.x + arm, dpad.y - half}, {dpad.x + arm, dpad.y + half}, right);

    int scale = std::max(2, (int)(unit / 150.0f));
    fillCircle(renderer, buttonA.x, buttonA.y, buttonA.r, (pressed & ButtonA) ? active : base);
    fillCircle(renderer, buttonB.x, buttonB.y, buttonB.r, (pressed & ButtonB) ? active : base);
    drawText(renderer, "A", (int)buttonA.x, (int)(buttonA.y - 3.5f * scale), scale, label, true);
    drawText(renderer, "B", (int)buttonB.x, (int)(buttonB.y - 3.5f * scale), scale, label, true);

    int small = std::max(1, scale / 2);
    fillRoundRect(renderer, buttonSelect.x, buttonSelect.y, buttonSelect.w, buttonSelect.h, (pressed & ButtonSelect) ? active : base);
    fillRoundRect(renderer, buttonStart.x, buttonStart.y, buttonStart.w, buttonStart.h, (pressed & ButtonStart) ? active : base);
    drawText(renderer, "SELECT", (int)(buttonSelect.x + buttonSelect.w / 2), (int)(buttonSelect.y + buttonSelect.h / 2 - 3.5f * small), small, label, true);
    drawText(renderer, "START", (int)(buttonStart.x + buttonStart.w / 2), (int)(buttonStart.y + buttonStart.h / 2 - 3.5f * small), small, label, true);
}
}
