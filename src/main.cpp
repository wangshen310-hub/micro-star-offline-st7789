#include <Arduino.h>
#include <WiFi.h>

#include "LGFX.h"
#include "LocationConfig.h"
#include "SerialTimeSync.h"
#include "StarMap.h"

constexpr int SCREEN_SIZE = 240;

LGFX tft;
LGFX_Sprite backbuffer(&tft);
LocationConfig locationConfig;
SerialTimeSync timeSync;
StarMap starMap(tft, backbuffer);
bool locationStarted = false;

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.setBrightness(255);
    backbuffer.setColorDepth(8);
    backbuffer.createSprite(SCREEN_SIZE, SCREEN_SIZE);

    // Offline mode: keep the radio disabled and never connect to saved networks.
    WiFi.mode(WIFI_OFF);

    locationStarted = locationConfig.Begin();
    if (locationStarted) {
        Serial.print("LOCATION ");
        Serial.print(locationConfig.Latitude(), 6);
        Serial.print(" ");
        Serial.println(locationConfig.Longitude(), 6);
        starMap.Begin(locationConfig.Latitude(), locationConfig.Longitude());
    } else {
        Serial.println("LOCATION_REQUIRED");
        starMap.ShowLocationRequired();
    }

    timeSync.SetLocationConfig(locationConfig);
    timeSync.Begin();
}

void loop()
{
    timeSync.Update();
    if (!locationStarted && locationConfig.IsConfigured()) {
        locationStarted = true;
        Serial.print("LOCATION ");
        Serial.print(locationConfig.Latitude(), 6);
        Serial.print(" ");
        Serial.println(locationConfig.Longitude(), 6);
        starMap.Begin(locationConfig.Latitude(), locationConfig.Longitude());
    }

    if (locationStarted)
        starMap.Update(timeSync.IsTimeValid(), timeSync.TimezoneOffsetMinutes());
    delay(20);
}
