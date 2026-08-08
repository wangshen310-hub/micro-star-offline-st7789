#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "LGFX.h"
#include "SerialTimeSync.h"
#include "StarMap.h"

constexpr int SCREEN_SIZE = 240;
constexpr double DEFAULT_LATITUDE = 31.771894;
constexpr double DEFAULT_LONGITUDE = 117.198016;

LGFX tft;
LGFX_Sprite backbuffer(&tft);
Preferences preferences;
SerialTimeSync timeSync;
StarMap starMap(tft, backbuffer);

void setup()
{
    Serial.begin(115200);

    tft.init();
    tft.setBrightness(255);
    backbuffer.setColorDepth(8);
    backbuffer.createSprite(SCREEN_SIZE, SCREEN_SIZE);

    // Offline mode: keep the radio disabled and never connect to saved networks.
    WiFi.mode(WIFI_OFF);

    preferences.begin("config", true);
    const double latitude = preferences.getString("latitude", String(DEFAULT_LATITUDE, 6)).toDouble();
    const double longitude = preferences.getString("longitude", String(DEFAULT_LONGITUDE, 6)).toDouble();
    preferences.end();

    Serial.print("LOCATION ");
    Serial.print(latitude, 6);
    Serial.print(" ");
    Serial.println(longitude, 6);

    starMap.Begin(latitude, longitude);
    timeSync.Begin();
}

void loop()
{
    timeSync.Update();
    starMap.Update(timeSync.IsTimeValid(), timeSync.TimezoneOffsetMinutes());
    delay(20);
}
