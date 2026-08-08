#pragma once

#include <Arduino.h>

class SerialTimeSync
{
public:
    void Begin();
    void Update();
    bool IsTimeValid() const;
    int TimezoneOffsetMinutes() const;

private:
    String input;
    int timezoneOffsetMinutes = 480;

    void HandleLine(const String& line);
};
