#include <Arduino.h>

#include "LGFX.h"

namespace {

LGFX display;

constexpr std::uint32_t kColors[] = {
    TFT_RED,
    TFT_GREEN,
    TFT_BLUE,
    TFT_WHITE,
    TFT_BLACK,
};

constexpr const char* kColorNames[] = {
    "red",
    "green",
    "blue",
    "white",
    "black",
};

std::size_t colorIndex = 0;
std::uint32_t nextColorAt = 0;
std::uint32_t nextHeartbeatAt = 0;

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("factory-display-diagnostic: boot");

    display.init();
    display.setRotation(0);
    display.setBrightness(255);
    display.fillScreen(kColors[colorIndex]);
    Serial.printf(
        "factory-display-diagnostic: panel=%dx%d color=%s\n",
        display.width(),
        display.height(),
        kColorNames[colorIndex]
    );

    nextColorAt = millis() + 2000;
    nextHeartbeatAt = millis() + 1000;
}

void loop()
{
    const std::uint32_t now = millis();

    if (static_cast<std::int32_t>(now - nextHeartbeatAt) >= 0) {
        Serial.printf("factory-display-diagnostic: heartbeat ms=%lu\n", now);
        nextHeartbeatAt += 1000;
    }

    if (static_cast<std::int32_t>(now - nextColorAt) >= 0) {
        colorIndex = (colorIndex + 1) % (sizeof(kColors) / sizeof(kColors[0]));
        display.fillScreen(kColors[colorIndex]);
        Serial.printf(
            "factory-display-diagnostic: color=%s\n",
            kColorNames[colorIndex]
        );
        nextColorAt += 2000;
    }

    delay(5);
}
