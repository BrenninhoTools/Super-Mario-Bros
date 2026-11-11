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
const SDL_Color kRunGlow = {90, 230, 100, 255};
const SDL_Color kGold = {244, 186, 24, 255};
const SDL_Color kGoldPressed = {196, 140, 10, 255};
const SDL_Color kGoldDark = {130, 84, 0, 255};
const SDL_Color kGoldShine = {255, 232, 150, 255};
const SDL_Color kDarkLabel = {60, 36, 0, 255};
}

void TouchControls::layout(int width, int height, float insetLeft, float insetRight, float insetBottom)
{
    unit = (float)std::min(width, height);
    float margin = unit * 0.06f;
    float left = std::max(margin, insetLeft);
    float right = std::max(margin, insetRight);
    float bottom = std::max(margin, insetBottom);
    stick.r = unit * 0.17f;
    stick.x = left + stick.r;
    stick.y = height - bottom - stick.r;
    buttonA.r = unit * 0.085f;
    buttonA.x = width - right - buttonA.r;
    buttonA.y = height - bottom - buttonA.r * 2.4f;
    buttonB.r = buttonA.r;
    buttonB.x = buttonA.x - buttonA.r * 2.5f;
    buttonB.y = height - bottom - buttonB.r;
    buttonC.r = buttonA.r;
    buttonC.x = buttonB.x;
    buttonC.y = buttonB.y - buttonB.r * 2.6f;
    buttonSelect.w = unit * 0.15f;
    buttonSelect.h = unit * 0.05f;
    buttonStart = buttonSelect;
    buttonSelect.x = width * 0.5f - buttonSelect.w - unit * 0.035f;
    buttonStart.x = width * 0.5f + unit * 0.035f;
    buttonSelect.y = buttonStart.y = height - std::max(margin * 0.6f, insetBottom) - buttonSelect.h;
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
        if (!stickActive && inCircle(stick, point.x, point.y, 1.6f))
        {
            stickActive = true;
            stickFinger = event.tfinger.fingerId;
            stickPoint = point;
        }
        if (visible && inCircle(buttonC, point.x, point.y, 1.25f))
        {
            runLatched = !runLatched;
        }
        if (visible && runLatched && inCircle(buttonB, point.x, point.y, 1.3f))
        {
            bReleaseUntil = SDL_GetTicks() + 40;
        }
        break;
    }
    case SDL_FINGERMOTION:
        fingers[event.tfinger.fingerId] = {event.tfinger.x * outputWidth, event.tfinger.y * outputHeight};
        if (stickActive && event.tfinger.fingerId == stickFinger)
        {
            stickPoint = fingers[event.tfinger.fingerId];
        }
        break;
    case SDL_FINGERUP:
        fingers.erase(event.tfinger.fingerId);
        if (stickActive && event.tfinger.fingerId == stickFinger)
        {
            stickActive = false;
        }
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

SDL_FPoint TouchControls::knobOffset() const
{
    if (!stickActive)
    {
        return {0.0f, 0.0f};
    }
    float dx = stickPoint.x - stick.x;
    float dy = stickPoint.y - stick.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    float limit = stick.r * 0.55f;
    if (distance > limit)
    {
        dx = dx / distance * limit;
        dy = dy / distance * limit;
    }
    return {dx, dy};
}

uint8_t TouchControls::stickButtons() const
{
    if (!stickActive)
    {
        return 0;
    }
    float dx = stickPoint.x - stick.x;
    float dy = stickPoint.y - stick.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance < stick.r * 0.22f)
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
    if (inCircle(buttonA, x, y, 1.3f))
    {
        result |= ButtonA;
    }
    if (inCircle(buttonB, x, y, 1.3f))
    {
        result |= ButtonB;
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
    uint8_t result = (runLatched ? ButtonB : 0) | stickButtons();
    for (const auto& finger : fingers)
    {
        if (stickActive && finger.first == stickFinger)
        {
            continue;
        }
        result |= buttonsAt(finger.second.x, finger.second.y);
    }
    if (runLatched && !SDL_TICKS_PASSED(SDL_GetTicks(), bReleaseUntil))
    {
        result &= (uint8_t)~ButtonB;
    }
    return result;
}

void TouchControls::drawStick(SDL_Renderer* renderer, uint8_t pressed) const
{
    fillCircle(renderer, stick.x, stick.y, stick.r * 1.08f, kPlateEdge);
    fillCircle(renderer, stick.x, stick.y, stick.r, kPlate);
    fillCircle(renderer, stick.x, stick.y, stick.r * 0.86f, kCross);

    float tip = stick.r * 0.8f;
    float base = stick.r * 0.66f;
    float wing = stick.r * 0.1f;
    fillTriangle(renderer, {stick.x, stick.y - tip}, {stick.x - wing, stick.y - base}, {stick.x + wing, stick.y - base}, (pressed & ButtonUp) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {stick.x, stick.y + tip}, {stick.x + wing, stick.y + base}, {stick.x - wing, stick.y + base}, (pressed & ButtonDown) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {stick.x - tip, stick.y}, {stick.x - base, stick.y + wing}, {stick.x - base, stick.y - wing}, (pressed & ButtonLeft) ? kArrowPressed : kArrow);
    fillTriangle(renderer, {stick.x + tip, stick.y}, {stick.x + base, stick.y - wing}, {stick.x + base, stick.y + wing}, (pressed & ButtonRight) ? kArrowPressed : kArrow);

    SDL_FPoint offset = knobOffset();
    float kx = stick.x + offset.x;
    float ky = stick.y + offset.y;
    float knob = stick.r * 0.42f;
    fillCircle(renderer, kx, ky + knob * 0.08f, knob * 1.04f, kRedDark);
    fillCircle(renderer, kx, ky, knob, stickActive ? kRedPressed : kRed);
    fillCircle(renderer, kx, ky, knob * 0.62f, kRedDark);
    fillCircle(renderer, kx, ky, knob * 0.54f, stickActive ? kRedPressed : kRed);
    fillCircle(renderer, kx - knob * 0.4f, ky - knob * 0.42f, knob * 0.16f, kRedShine);
}

void TouchControls::drawRoundButton(SDL_Renderer* renderer, const Circle& c, const char* label, const ButtonStyle& style, bool pressed, bool latched) const
{
    int scale = std::max(2, (int)(c.r / 11.0f));
    if (latched)
    {
        fillCircle(renderer, c.x, c.y, c.r * 1.32f, kRunGlow);
    }
    fillCircle(renderer, c.x, c.y, c.r * 1.18f, kPlate);
    fillCircle(renderer, c.x, c.y, c.r * 1.04f, style.rim);
    float offset = pressed ? c.r * 0.06f : 0.0f;
    fillCircle(renderer, c.x, c.y + offset, c.r * 0.94f, pressed ? style.pressed : style.face);
    if (!pressed)
    {
        fillCircle(renderer, c.x - c.r * 0.32f, c.y - c.r * 0.34f, c.r * 0.2f, style.shine);
    }
    drawText(renderer, label, (int)c.x, (int)(c.y + offset - 3.5f * scale), scale, style.label, true);
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

TouchControls::~TouchControls()
{
    if (layer)
    {
        SDL_DestroyTexture(layer);
    }
}

void TouchControls::drawControls(SDL_Renderer* renderer) const
{
    static const ButtonStyle red = {kRed, kRedPressed, kRedDark, kRedShine, kWhite};
    static const ButtonStyle gold = {kGold, kGoldPressed, kGoldDark, kGoldShine, kDarkLabel};
    uint8_t pressed = buttons();
    uint8_t touched = 0;
    bool touchingC = false;
    for (const auto& finger : fingers)
    {
        touched |= buttonsAt(finger.second.x, finger.second.y);
        touchingC |= inCircle(buttonC, finger.second.x, finger.second.y, 1.25f);
    }
    drawStick(renderer, pressed);
    drawRoundButton(renderer, buttonC, "C", gold, touchingC, runLatched);
    drawRoundButton(renderer, buttonB, "B", red, (touched & ButtonB) != 0, false);
    drawRoundButton(renderer, buttonA, "A", red, (pressed & ButtonA) != 0, false);
    drawPill(renderer, buttonSelect, "SELECT", (pressed & ButtonSelect) != 0);
    drawPill(renderer, buttonStart, "START", (pressed & ButtonStart) != 0);
}

void TouchControls::draw(SDL_Renderer* renderer, float opacity)
{
    if (!visible)
    {
        return;
    }
    if (opacity >= 0.99f)
    {
        drawControls(renderer);
        return;
    }
    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    if (!layer || layerWidth != width || layerHeight != height)
    {
        if (layer)
        {
            SDL_DestroyTexture(layer);
        }
        layer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
        layerWidth = width;
        layerHeight = height;
        if (layer)
        {
            SDL_SetTextureBlendMode(layer, SDL_BLENDMODE_BLEND);
        }
    }
    if (!layer || SDL_SetRenderTarget(renderer, layer) != 0)
    {
        drawControls(renderer);
        return;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    drawControls(renderer);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetTextureAlphaMod(layer, (Uint8)(opacity * 255.0f));
    SDL_RenderCopy(renderer, layer, nullptr, nullptr);
}
}
