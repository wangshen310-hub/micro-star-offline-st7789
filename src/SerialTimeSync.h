#pragma once

#include <Arduino.h>

#include "LocationConfig.h"

class SerialTimeSync
{
public:
    void SetLocationConfig(LocationConfig& config);
    void Begin();
    void Update();
    bool IsTimeValid() const;
    int TimezoneOffsetMinutes() const;

private:
    String input;
    int timezoneOffsetMinutes = 480;
    LocationConfig* locationConfig = nullptr;

    void HandleLine(const String& line);
};
