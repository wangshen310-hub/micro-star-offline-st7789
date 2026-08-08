#pragma once

#include <Arduino.h>

#include "LGFX.h"

extern "C" {
#include <astronomy.h>
}

struct SkyObject
{
    const char* name;
    astro_body_t body;
    uint16_t color;
    double azimuth;
    double altitude;
    double magnitude;
    double illuminatedFraction;
    bool valid;
};

class StarMap
{
public:
    StarMap(LGFX& display, LGFX_Sprite& canvas);

    void Begin(double latitude, double longitude);
    void Update(bool timeValid, int timezoneOffsetMinutes);

private:
    LGFX& tft;
    LGFX_Sprite& backbuffer;
    astro_observer_t observer{};
    SkyObject objects[5]{};
    unsigned long lastCalculation = 0;
    bool hasCalculation = false;
    int labelBoxCount = 0;
    int labelBoxX[16]{};
    int labelBoxY[16]{};
    int labelBoxWidth[16]{};
    int labelBoxHeight[16]{};

    void Calculate();
    void DrawWaitingScreen();
    void DrawChart(int timezoneOffsetMinutes);
    void DrawNightStars();
    void DrawGrid();
    void DrawObject(const SkyObject& object);
    void DrawMarker(const SkyObject& object);
    std::pair<int, int> Project(double azimuth, double altitude) const;
};
