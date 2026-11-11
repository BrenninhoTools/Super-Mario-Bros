#pragma once

#include <cstdint>

namespace smb
{
enum Button : uint8_t
{
    ButtonA = 0x01,
    ButtonB = 0x02,
    ButtonSelect = 0x04,
    ButtonStart = 0x08,
    ButtonUp = 0x10,
    ButtonDown = 0x20,
    ButtonLeft = 0x40,
    ButtonRight = 0x80
};

class Controller
{
public:
    void setButtons(uint8_t buttons)
    {
        if ((buttons & ButtonLeft) && (buttons & ButtonRight))
        {
            buttons &= (uint8_t)~(ButtonLeft | ButtonRight);
        }
        if ((buttons & ButtonUp) && (buttons & ButtonDown))
        {
            buttons &= (uint8_t)~(ButtonUp | ButtonDown);
        }
        state = buttons;
    }

    void strobe(uint8_t value)
    {
        strobing = value & 1;
        if (strobing)
        {
            shift = state;
        }
    }

    uint8_t read()
    {
        if (strobing)
        {
            return state & 1;
        }
        uint8_t bit = shift & 1;
        shift = (uint8_t)((shift >> 1) | 0x80);
        return bit;
    }

private:
    uint8_t state = 0;
    uint8_t shift = 0;
    bool strobing = false;
};
}
