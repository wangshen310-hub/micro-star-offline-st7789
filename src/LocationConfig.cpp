#include "LocationConfig.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>

bool LocationConfig::Begin()
{
    preferences.begin("config", true);
    const String latitudeText = preferences.getString("latitude", "");
    const String longitudeText = preferences.getString("longitude", "");
    preferences.end();

    if (latitudeText.isEmpty() || longitudeText.isEmpty()) {
        configured = false;
        return false;
    }

    char* latitudeEnd = nullptr;
    char* longitudeEnd = nullptr;
    const double storedLatitude = strtod(latitudeText.c_str(), &latitudeEnd);
    const double storedLongitude = strtod(longitudeText.c_str(), &longitudeEnd);
    if (latitudeEnd == latitudeText.c_str() || *latitudeEnd != '\0'
        || longitudeEnd == longitudeText.c_str() || *longitudeEnd != '\0') {
        configured = false;
        return false;
    }

    return Apply(storedLatitude, storedLongitude, false);
}

bool LocationConfig::IsConfigured() const
{
    return configured;
}

double LocationConfig::Latitude() const
{
    return latitude;
}

double LocationConfig::Longitude() const
{
    return longitude;
}

bool LocationConfig::HandleCommand(const String& rawLine)
{
    String line = rawLine;
    line.trim();
    if (!line.startsWith("SETLOC"))
        return false;

    double newLatitude = 0.0;
    double newLongitude = 0.0;
    if (sscanf(line.c_str(), "SETLOC %lf %lf", &newLatitude, &newLongitude) != 2) {
        Serial.println("LOCATION_ERROR FORMAT");
        return true;
    }

    if (!Apply(newLatitude, newLongitude, true)) {
        Serial.println("LOCATION_ERROR RANGE");
        return true;
    }

    Serial.print("LOCATION_OK ");
    Serial.print(latitude, 6);
    Serial.print(" ");
    Serial.println(longitude, 6);
    return true;
}

bool LocationConfig::Apply(double newLatitude, double newLongitude, bool persist)
{
    if (!IsValid(newLatitude, newLongitude)) {
        configured = false;
        return false;
    }

    latitude = newLatitude;
    longitude = newLongitude;
    configured = true;

    if (persist) {
        preferences.begin("config", false);
        preferences.putString("latitude", String(latitude, 6));
        preferences.putString("longitude", String(longitude, 6));
        preferences.end();
    }
    return true;
}

bool LocationConfig::IsValid(double newLatitude, double newLongitude)
{
    return std::isfinite(newLatitude) && std::isfinite(newLongitude)
        && newLatitude >= -90.0 && newLatitude <= 90.0
        && newLongitude >= -180.0 && newLongitude <= 180.0;
}
