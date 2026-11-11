#include "TouchControls.hpp"

#include <algorithm>
#include <cmath>

#include "Controller.hpp"
#include "Draw.hpp"

namespace smb
{
namespace
{
const SDL_Color kPlate = {178, 178, 178, 255};
const SDL_Color kPlateEdge = {120, 120, 120, 255};
const SDL_Color kCross = {28, 28, 28, 255};
const SDL_Color kCrossPressed = {70, 70, 70, 255};
const SDL_Color kArrow = {90, 90, 90, 255};
const SDL_Color kArrowPressed = {255, 205, 40, 255};
const SDL_Color kRed = {214, 32, 44, 255};
const SDL_Color kRedDark = {120, 12, 22, 255};
const SDL_Color kRedPressed = {150, 18, 28, 255};
const SDL_Color kRedShine = {255, 130, 130, 255};
const SDL_Color kLabelRed = {232, 36, 48, 255};
const SDL_Color kWhite = {255, 255, 255, 255};
const SDL_Color kPillDark = {34, 34, 34, 255};
const SDL_Color kPillPressed = {90, 90, 90, 255};
const SDL_Color kRunGlow = {255, 205, 40, 255};

void fillRect(SDL_Renderer* renderer, float x, float y, float w, float h, SDL_Color color)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRectF(renderer, &rect);
}
}

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
    buttonSelect.w = unit * 0.15f;
    buttonSelect.h = unit * 0.05f;
    buttonStart = buttonSelect;
    buttonSelect.x = width * 0.5f - buttonSelect.w - unit * 0.035f;
    buttonStart.x = width * 0.5f + unit * 0.035f;
    buttonSelect.y = buttonStart.y = height - margin * 0.6f - buttonSelect.h;
}

bool TouchControls::inCircle(const Circle& c, float x, float y, float scale) const
{
    float dx = x - c.x;
    float dy = y - c.y;
    return dx * dx + dy * dy <= c.r * c.r * scale * scale;
}

bool TouchControls::inPill(const Pill& p, float x, float y) const
{
    float pad = unit * 0.025f;
    return x >= p.x - pad && x <= p.x + p.w + pad && y >= p.y - pad * 2 && y <= p.y + p.h + pad;
}

void TouchControls::handleEvent(const SDL_Event& event, int outputWidth, int outputHeight)
{
    switch (event.type)
    {
    case SDL_FINGERDOWN:
    {
        pendingTap = true;
        SDL_FPoint point = {event.tfinger.x * outputWidth, event.tfinger.y * outputHeight};
        fingers[event.tfinger.fingerId] = point;
        if (visible && inCircle(buttonB, point.x, point.y, 1.35f))
        {
            runLatched = !runLatched;
        }
        break;
    }
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
    if (inCircle(buttonA, x, y, 1.35f))
    {
        result |= ButtonA;
    }
    if (inPill(buttonSelect, x, y))
    {
        result |= ButtonSelect;
    }
    if (inPill(buttonStart, x, y))
    {
        result |= ButtonStart;
    }
    return result;
}

uint8_t TouchControls::buttons() const
{
    uint8_t result = runLatched ? ButtonB : 0;
    for (const auto& finger : fingers)
    {
        result |= dpadAt(finger.second.x, finger.second.y);
        result |= buttonsAt(finger.second.x, finger.second.y);
    }
    return result;
}

void TouchControls::drawDpad(SDL_Renderer* renderer, uint8_t pressed) const
{
    fillCircle(renderer, dpad.x, dpad.y, dpad.r * 1.08f, kPlateEdge);
    fillCircle(renderer, dpad.x, dpad.y, dpad.r, kPlate);

    float arm = dpad.r * 0.86f;
    float half = dpad.r * 0.29f;
    fillRect(renderer, dpad.x - half, dpad.y - arm, half * 2, arm * 2, kCross);
    fillRect(renderer, dpad.x - arm, dpad.y - half, arm - half, half * 2, kCross);
    fillRect(renderer, dpad.x + half, dpad.y - half, arm - half, half * 2, kCross);

    float inner = half * 0.85f;
    if (pressed & ButtonUp)
    {
        fillRect(renderer, dpad.x - inner, dpad.y - arm + half * 0.15f, inner * 2, arm - half, kCrossPressed);
    }
    if (pressed & ButtonDown)
    {
        fillRect(renderer, dpad.x - inner, dpad.y + half, inner * 2, arm - half - half * 0.15f, kCrossPressed);
    }
    if (pressed & ButtonLeft)
    {
        fillRect(renderer, dpad.x - arm + half * 0.15f, dpad.y - inner, arm - half, inner * 2, kCrossPressed);
    }
    if (pressed & ButtonRight)
    {
        fillRect(renderer, dpad.x + half, dpad.y - inner, arm - half - half * 0.15f, inner * 2, kCrossPressed);
    }

    fillCircle(renderer, dpad.x, dpad.y, half * 0.55f, kCrossPressed);

    float tip = arm * 0.88f;
    float base = arm * 0.55f;
    float wing = half * 0.55f;
    fillTriangle(renderer, {dpad.x, dpad.y - tip}, {dpad.x - wing, dpad.y - base}, {dpad.x + wing, dpad.y - base}, (pressed & ButtonUp) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {dpad.x, dpad.y + tip}, {dpad.x + wing, dpad.y + base}, {dpad.x - wing, dpad.y + base}, (pressed & ButtonDown) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {dpad.x - tip, dpad.y}, {dpad.x - base, dpad.y + wing}, {dpad.x - base, dpad.y - wing}, (pressed & ButtonLeft) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {dpad.x + tip, dpad.y}, {dpad.x + base, dpad.y - wing}, {dpad.x + base, dpad.y + wing}, (pressed & ButtonRight) ? kArrowPressed : kArrow);
}

void TouchControls::drawRoundButton(SDL_Renderer* renderer, const Circle& c, const char* label, bool pressed, bool latched) const
{
    int scale = std::max(2, (int)(c.r / 11.0f));
    if (latched)
    {
        fillCircle(renderer, c.x, c.y, c.r * 1.32f, kRunGlow);
    }
    fillCircle(renderer, c.x, c.y, c.r * 1.18f, kPlate);
    fillCircle(renderer, c.x, c.y, c.r * 1.04f, kRedDark);
    float offset = pressed ? c.r * 0.06f : 0.0f;
    fillCircle(renderer, c.x, c.y + offset, c.r * 0.94f, pressed ? kRedPressed : kRed);
    if (!pressed)
    {
        fillCircle(renderer, c.x - c.r * 0.32f, c.y - c.r * 0.34f, c.r * 0.2f, kRedShine);
    }
    drawText(renderer, label, (int)c.x, (int)(c.y + offset - 3.5f * scale), scale, kWhite, true);
    if (latched)
    {
        int small = std::max(1, scale / 2);
        drawText(renderer, "RUN", (int)c.x, (int)(c.y - c.r * 1.45f - 8 * small), small, kRunGlow, true);
    }
}

void TouchControls::drawPill(SDL_Renderer* renderer, const Pill& p, const char* label, bool pressed) const
{
    int scale = std::max(1, (int)(p.h / 12.0f));
    drawText(renderer, label, (int)(p.x + p.w / 2), (int)(p.y - 9 * scale), scale, kLabelRed, true);
    float rim = p.h * 0.18f;
    fillRoundRect(renderer, p.x - rim, p.y - rim, p.w + rim * 2, p.h + rim * 2, kPlate);
    fillRoundRect(renderer, p.x, p.y, p.w, p.h, pressed ? kPillPressed : kPillDark);
}

void TouchControls::draw(SDL_Renderer* renderer) const
{
    if (!visible)
    {
        return;
    }
    uint8_t pressed = buttons();
    uint8_t touched = 0;
    for (const auto& finger : fingers)
    {
        touched |= buttonsAt(finger.second.x, finger.second.y);
        if (inCircle(buttonB, finger.second.x, finger.second.y, 1.35f))
        {
            touched |= ButtonB;
        }
    }
    drawDpad(renderer, pressed);
    drawRoundButton(renderer, buttonB, "B", (touched & ButtonB) != 0, runLatched);
    drawRoundButton(renderer, buttonA, "A", (pressed & ButtonA) != 0, false);
    drawPill(renderer, buttonSelect, "SELECT", (pressed & ButtonSelect) != 0);
    drawPill(renderer, buttonStart, "START", (pressed & ButtonStart) != 0);
}
}
