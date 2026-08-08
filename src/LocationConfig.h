#pragma once

#include <Arduino.h>
#include <Preferences.h>

class LocationConfig
{
public:
    bool Begin();
    bool IsConfigured() const;
    double Latitude() const;
    double Longitude() const;

    // Handles SETLOC <latitude> <longitude>. Returns true for any SETLOC line.
    bool HandleCommand(const String& line);

private:
    Preferences preferences;
    bool configured = false;
    double latitude = 0.0;
    double longitude = 0.0;

    bool Apply(double newLatitude, double newLongitude, bool persist);
    static bool IsValid(double newLatitude, double newLongitude);
};
