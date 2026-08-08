#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

namespace lgfx {

inline std::uint16_t color888(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue
)
{
    return static_cast<std::uint16_t>((red & 0xF8) << 8)
        | static_cast<std::uint16_t>((green & 0xFC) << 3)
        | static_cast<std::uint16_t>(blue >> 3);
}

}  // namespace lgfx

class LGFX : public TFT_eSPI
{
public:
    void init()
    {
        pinMode(1, OUTPUT);
        analogWrite(1, 255);

        begin();
        invertDisplay(true);
        setRotation(0);
    }

    void setBrightness(std::uint8_t brightness)
    {
        analogWrite(1, 255 - brightness);
    }

    using TFT_eSPI::drawCentreString;

    std::int16_t drawCentreString(
        const char* text,
        std::int32_t x,
        std::int32_t y
    )
    {
        return TFT_eSPI::drawCentreString(text, x, y, 1);
    }

    std::int16_t drawCenterString(
        const char* text,
        std::int32_t x,
        std::int32_t y
    )
    {
        return drawCentreString(text, x, y);
    }
};

using LGFX_Sprite = TFT_eSprite;
