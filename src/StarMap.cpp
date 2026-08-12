#include "StarMap.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <time.h>

namespace {
constexpr int SCREEN_SIZE = 240;
constexpr int HORIZON_Y = 180;
constexpr int SKY_UPPER_HEIGHT = 150;
constexpr int SKY_LOWER_HEIGHT = 47;
constexpr int CHART_MARGIN = 8;
constexpr int AZIMUTH_SPAN = SCREEN_SIZE - 1 - 2 * CHART_MARGIN;
constexpr unsigned long CALCULATION_INTERVAL_MS = 30000;
constexpr double PI_VALUE = 3.14159265358979323846;
constexpr double HORIZON_TRANSITION_DEGREES = 4.0;
constexpr double LABEL_APPEAR_DEGREES = 1.25;

struct NightStarPoint
{
    uint8_t x;
    uint8_t y;
    uint8_t size;
};

constexpr NightStarPoint NIGHT_STARS[] = {
    { 14, 35, 1 }, { 31, 62, 1 }, { 48, 28, 2 }, { 66, 86, 1 },
    { 82, 46, 1 }, { 99, 104, 2 }, { 116, 34, 1 }, { 134, 73, 1 },
    { 151, 24, 2 }, { 169, 97, 1 }, { 187, 51, 1 }, { 207, 114, 2 },
    { 226, 39, 1 }, { 22, 127, 1 }, { 43, 151, 2 }, { 61, 112, 1 },
    { 78, 139, 1 }, { 96, 68, 1 }, { 114, 157, 2 }, { 132, 119, 1 },
    { 149, 145, 1 }, { 166, 57, 2 }, { 183, 132, 1 }, { 201, 79, 1 },
    { 219, 156, 2 }, { 27, 94, 1 }, { 54, 43, 1 }, { 72, 163, 1 },
    { 105, 126, 1 }, { 141, 42, 1 }, { 176, 160, 1 }, { 214, 67, 1 }
};

int BaseRadius(astro_body_t body)
{
    if (body == BODY_SUN)
        return 7;
    if (body == BODY_MOON || body == BODY_VENUS || body == BODY_JUPITER || body == BODY_SATURN)
        return 6;
    if (body == BODY_MERCURY)
        return 4;
    return 5;
}

bool HasMagnitude(astro_body_t body)
{
    return body == BODY_MERCURY || body == BODY_VENUS || body == BODY_MARS
        || body == BODY_JUPITER || body == BODY_SATURN
        || body == BODY_URANUS || body == BODY_NEPTUNE;
}

double VisibilityFactor(double altitude)
{
    const double linear = std::max(0.0, std::min(1.0, altitude / HORIZON_TRANSITION_DEGREES));
    return linear * linear * (3.0 - 2.0 * linear);
}

uint16_t Rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return lgfx::color888(red, green, blue);
}
}

StarMap::StarMap(LGFX& display, LGFX_Sprite& canvas)
    : tft(display), backbuffer(canvas)
{
}

void StarMap::Begin(double latitude, double longitude)
{
    observer = Astronomy_MakeObserver(latitude, longitude, 0.0);
    initialized = true;
    hasCalculation = false;
    objects[0] = { "SUN", BODY_SUN, Rgb(255, 190, 0), 0, 0, 0, 1, false };
    objects[1] = { "MOON", BODY_MOON, Rgb(210, 220, 255), 0, 0, 0, 0, false };
    objects[2] = { "MERCURY", BODY_MERCURY, Rgb(175, 165, 150), 0, 0, 0, 0, false };
    objects[3] = { "VENUS", BODY_VENUS, Rgb(255, 225, 155), 0, 0, 0, 0, false };
    objects[4] = { "MARS", BODY_MARS, Rgb(255, 70, 30), 0, 0, 0, 0, false };
    objects[5] = { "JUPITER", BODY_JUPITER, Rgb(235, 178, 105), 0, 0, 0, 0, false };
    objects[6] = { "SATURN", BODY_SATURN, Rgb(230, 205, 125), 0, 0, 0, 0, false };
    objects[7] = { "URANUS", BODY_URANUS, Rgb(115, 220, 225), 0, 0, 0, 0, false };
    objects[8] = { "NEPTUNE", BODY_NEPTUNE, Rgb(75, 115, 255), 0, 0, 0, 0, false };
}

void StarMap::Update(bool timeValid, int timezoneOffsetMinutes)
{
    if (!initialized) {
        ShowLocationRequired();
        return;
    }

    if (!timeValid) {
        DrawWaitingScreen();
        return;
    }

    const unsigned long now = millis();
    if (!hasCalculation || now - lastCalculation >= CALCULATION_INTERVAL_MS) {
        Calculate();
        lastCalculation = now;
        hasCalculation = true;
    }

    DrawChart(timezoneOffsetMinutes);
}

void StarMap::ShowLocationRequired()
{
    backbuffer.fillScreen(Rgb(0, 0, 8));
    backbuffer.setTextColor(Rgb(110, 180, 255));
    backbuffer.setTextSize(2);
    backbuffer.drawCentreString("LOCATION REQUIRED", 120, 28, 1);
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(Rgb(220, 220, 220));
    backbuffer.drawCentreString("Connect USB serial", 120, 78, 1);
    backbuffer.drawCentreString("SETLOC <LAT> <LON>", 120, 100, 1);
    backbuffer.drawCentreString("Example:", 120, 126, 1);
    backbuffer.drawCentreString("SETLOC 31.2304 121.4737", 120, 145, 1);
    backbuffer.setTextColor(Rgb(100, 120, 150));
    backbuffer.drawCentreString("Coordinates are saved", 120, 175, 1);
    backbuffer.pushSprite(0, 0);
}

void StarMap::Calculate()
{
    astro_time_t time = Astronomy_CurrentTime();

    for (SkyObject& object : objects) {
        const astro_equatorial_t equatorial = Astronomy_Equator(
            object.body,
            &time,
            observer,
            EQUATOR_OF_DATE,
            ABERRATION
        );

        object.valid = equatorial.status == ASTRO_SUCCESS;
        if (!object.valid)
            continue;

        const astro_horizon_t horizon = Astronomy_Horizon(
            &time,
            observer,
            equatorial.ra,
            equatorial.dec,
            REFRACTION_NORMAL
        );
        object.azimuth = horizon.azimuth;
        object.altitude = horizon.altitude;

        if (object.body == BODY_MOON) {
            const astro_angle_result_t phase = Astronomy_MoonPhase(time);
            if (phase.status == ASTRO_SUCCESS)
                object.illuminatedFraction = (1.0 - cos(phase.angle * PI_VALUE / 180.0)) / 2.0;
        } else if (HasMagnitude(object.body)) {
            const astro_illum_t illumination = Astronomy_Illumination(object.body, time);
            if (illumination.status == ASTRO_SUCCESS) {
                object.illuminatedFraction = illumination.phase_fraction;
                object.magnitude = illumination.mag;
            }
        }
    }

    Serial.print("SKY");
    for (const SkyObject& object : objects) {
        Serial.print(" ");
        Serial.print(object.name);
        Serial.print(" ");
        Serial.print(object.azimuth, 2);
        Serial.print(" ");
        Serial.print(object.altitude, 2);
    }
    Serial.println();
}

void StarMap::DrawWaitingScreen()
{
    backbuffer.fillScreen(Rgb(0, 0, 8));
    backbuffer.setTextColor(Rgb(110, 180, 255));
    backbuffer.setTextSize(2);
    backbuffer.drawCentreString("MICRO STAR", 120, 38, 1);
    backbuffer.setTextSize(1);
    backbuffer.setTextColor(Rgb(220, 220, 220));
    backbuffer.drawCentreString("Waiting for USB time", 120, 92, 1);
    backbuffer.drawCentreString("Run StarTimeSync.cmd", 120, 112, 1);
    backbuffer.setTextColor(Rgb(100, 120, 150));
    backbuffer.drawCentreString("WiFi is OFF", 120, 145, 1);
    backbuffer.pushSprite(0, 0);
}

void StarMap::DrawChart(int timezoneOffsetMinutes)
{
    backbuffer.fillScreen(Rgb(0, 0, 8));
    labelBoxCount = 2;
    labelBoxX[0] = 0;
    labelBoxY[0] = HORIZON_Y - 4;
    labelBoxWidth[0] = SCREEN_SIZE;
    labelBoxHeight[0] = 34;
    labelBoxX[1] = 0;
    labelBoxY[1] = 0;
    labelBoxWidth[1] = 106;
    labelBoxHeight[1] = 24;

    for (const SkyObject& object : objects) {
        if (!object.valid || object.altitude <= 0.0 || labelBoxCount >= 24)
            continue;
        const auto [objectX, objectY] = Project(object.azimuth, object.altitude);
        const double visibility = VisibilityFactor(object.altitude);
        const int radius = std::max(1, static_cast<int>(round(BaseRadius(object.body) * visibility)));
        const int markerExtent = radius + (object.body == BODY_SATURN ? std::max(1, static_cast<int>(round(3.0 * visibility))) : 0);
        labelBoxX[labelBoxCount] = objectX - markerExtent - 2;
        labelBoxY[labelBoxCount] = objectY - markerExtent - 2;
        labelBoxWidth[labelBoxCount] = markerExtent * 2 + 5;
        labelBoxHeight[labelBoxCount] = markerExtent * 2 + 5;
        ++labelBoxCount;
    }

    DrawNightStars();
    DrawGrid();

    const time_t localEpoch = time(nullptr) + timezoneOffsetMinutes * 60;
    tm localTime{};
    gmtime_r(&localEpoch, &localTime);
    char timeText[10]{};
    strftime(timeText, sizeof(timeText), "%H:%M:%S", &localTime);
    backbuffer.fillRect(0, 0, 106, 24, Rgb(0, 0, 8));
    backbuffer.setTextColor(Rgb(170, 210, 235));
    backbuffer.setTextSize(2);
    backbuffer.drawString(timeText, 4, 3);

    for (const SkyObject& object : objects)
        DrawObject(object);
    for (const SkyObject& object : objects)
        DrawMarker(object);
    backbuffer.pushSprite(0, 0);
}

void StarMap::DrawNightStars()
{
    if (!objects[0].valid || objects[0].altitude >= 0.0)
        return;

    const double nightDepth = std::max(0.0, std::min(1.0, -objects[0].altitude / 12.0));
    const size_t visibleCount = static_cast<size_t>(round(nightDepth * (sizeof(NIGHT_STARS) / sizeof(NIGHT_STARS[0]))));
    const uint8_t brightness = static_cast<uint8_t>(55 + round(150.0 * nightDepth));

    for (size_t index = 0; index < visibleCount; ++index) {
        const NightStarPoint& star = NIGHT_STARS[index];
        if (star.x < 110 && star.y < 25)
            continue;
        const uint8_t variation = static_cast<uint8_t>((index % 3) * 12);
        const uint8_t level = static_cast<uint8_t>(std::min(235, brightness + variation));
        const uint16_t color = Rgb(level, level, static_cast<uint8_t>(std::min(255, level + 15)));
        if (star.size == 2)
            backbuffer.fillCircle(star.x, star.y, 1, color);
        else
            backbuffer.drawPixel(star.x, star.y, color);
    }
}
void StarMap::DrawGrid()
{
    const uint16_t horizon = Rgb(85, 170, 210);
    const uint16_t direction = Rgb(70, 130, 165);
    backbuffer.drawLine(0, HORIZON_Y, SCREEN_SIZE - 1, HORIZON_Y, horizon);

    const int ticks[] = { 8, 64, 120, 175, 231 };
    for (const int x : ticks)
        backbuffer.drawLine(x, HORIZON_Y - 3, x, HORIZON_Y + 3, direction);

    backbuffer.setTextColor(direction);
    backbuffer.setTextSize(1);
    backbuffer.drawCentreString("N", 8, HORIZON_Y + 10, 1);
    backbuffer.drawCentreString("W", 64, HORIZON_Y + 10, 1);
    backbuffer.drawCentreString("S", 120, HORIZON_Y + 10, 1);
    backbuffer.drawCentreString("E", 175, HORIZON_Y + 10, 1);
    backbuffer.drawCentreString("N", 231, HORIZON_Y + 10, 1);

    backbuffer.setTextColor(Rgb(55, 105, 135));
    backbuffer.drawCentreString("360", 8, HORIZON_Y + 20, 1);
    backbuffer.drawCentreString("270", 64, HORIZON_Y + 20, 1);
    backbuffer.drawCentreString("180", 120, HORIZON_Y + 20, 1);
    backbuffer.drawCentreString("90", 175, HORIZON_Y + 20, 1);
    backbuffer.drawCentreString("0", 231, HORIZON_Y + 20, 1);
}

std::pair<int, int> StarMap::Project(double azimuth, double altitude) const
{
    double normalizedAzimuth = fmod(azimuth, 360.0);
    if (normalizedAzimuth < 0.0)
        normalizedAzimuth += 360.0;
    const double signedAltitude = std::max(-90.0, std::min(90.0, altitude));
    const double altitudeRadians = signedAltitude * PI_VALUE / 180.0;
    const double verticalScale = signedAltitude >= 0.0 ? SKY_UPPER_HEIGHT : SKY_LOWER_HEIGHT;

    return {
        static_cast<int>(round(CHART_MARGIN + (360.0 - normalizedAzimuth) * AZIMUTH_SPAN / 360.0)),
        static_cast<int>(round(HORIZON_Y - verticalScale * sin(altitudeRadians)))
    };
}


void StarMap::DrawMarker(const SkyObject& object)
{
    if (!object.valid || object.altitude <= 0.0)
        return;

    const auto [x, y] = Project(object.azimuth, object.altitude);
    const double visibility = VisibilityFactor(object.altitude);
    const int radius = std::max(1, static_cast<int>(round(BaseRadius(object.body) * visibility)));
    int markerExtent = radius;

    if (object.body == BODY_SATURN) {
        const int ringExtension = std::max(1, static_cast<int>(round(3.0 * visibility)));
        const int ringHeight = std::max(1, static_cast<int>(round(2.0 * visibility)));
        markerExtent = radius + ringExtension;
        backbuffer.fillEllipse(x, y, markerExtent + 2, ringHeight + 2, Rgb(0, 0, 8));
        backbuffer.drawEllipse(x, y, markerExtent, ringHeight, object.color);
        backbuffer.drawEllipse(x, y, std::max(1, markerExtent - 1), ringHeight, object.color);
        backbuffer.fillCircle(x, y, std::max(1, radius - 2), object.color);
    } else {
        backbuffer.fillCircle(x, y, radius + 2, Rgb(0, 0, 8));
        backbuffer.fillCircle(x, y, radius, object.color);
    }

    const bool crossesHorizon = std::abs(y - HORIZON_Y) <= markerExtent + 2;
    if (!crossesHorizon) {
        backbuffer.drawLine(x - 3, HORIZON_Y, x + 3, HORIZON_Y, object.color);
        backbuffer.drawLine(x, HORIZON_Y - 2, x, HORIZON_Y + 2, object.color);
    }
}
void StarMap::DrawObject(const SkyObject& object)
{
    if (!object.valid || object.altitude < LABEL_APPEAR_DEGREES)
        return;

    const auto [x, y] = Project(object.azimuth, object.altitude);
    const int radius = std::max(1, static_cast<int>(round(BaseRadius(object.body) * VisibilityFactor(object.altitude))));

    char details[40]{};
    if (object.body == BODY_MOON) {
        snprintf(
            details,
            sizeof(details),
            "(A%03.0f H%+03.0f %2.0f%%)",
            object.azimuth,
            object.altitude,
            object.illuminatedFraction * 100.0
        );
    } else if (HasMagnitude(object.body)) {
        snprintf(
            details,
            sizeof(details),
            "(A%03.0f H%+03.0f M%+.1f)",
            object.azimuth,
            object.altitude,
            object.magnitude
        );
    } else {
        snprintf(
            details,
            sizeof(details),
            "(A%03.0f H%+03.0f)",
            object.azimuth,
            object.altitude
        );
    }

    char labelText[64]{};
    if (object.altitude < HORIZON_TRANSITION_DEGREES)
        snprintf(labelText, sizeof(labelText), "%s", object.name);
    else
        snprintf(labelText, sizeof(labelText), "%s %s", object.name, details);

    backbuffer.setTextColor(object.color);
    backbuffer.setTextSize(1);
    const int labelWidth = backbuffer.textWidth(labelText);
    constexpr int labelHeight = 9;
    constexpr int gap = 3;
    const int centeredX = x - labelWidth / 2;
    const int centeredY = y - labelHeight / 2;
    const int aboveY = y - radius - labelHeight - gap;
    const int belowY = y + radius + gap;
    const int extraBelowY = y + radius + labelHeight + 4;
    const int rightX = x + radius + gap;
    const int leftX = x - radius - gap - labelWidth;
    int candidateX[8]{};
    int candidateY[8]{};

    if (object.body == BODY_SUN) {
        const int xOrder[] = { centeredX, leftX, rightX, centeredX, leftX, rightX, rightX, leftX };
        const int yOrder[] = { aboveY, aboveY, aboveY, belowY, belowY, belowY, centeredY, centeredY };
        std::copy(std::begin(xOrder), std::end(xOrder), candidateX);
        std::copy(std::begin(yOrder), std::end(yOrder), candidateY);
    } else if (HasMagnitude(object.body)) {
        const int xOrder[] = { centeredX, rightX, leftX, centeredX, rightX, leftX, rightX, leftX };
        const int yOrder[] = { extraBelowY, extraBelowY, extraBelowY, aboveY, aboveY, aboveY, centeredY, centeredY };
        std::copy(std::begin(xOrder), std::end(xOrder), candidateX);
        std::copy(std::begin(yOrder), std::end(yOrder), candidateY);
    } else {
        const int xOrder[] = { rightX, leftX, centeredX, centeredX, rightX, leftX, rightX, leftX };
        const int yOrder[] = { centeredY, centeredY, aboveY, belowY, aboveY, aboveY, belowY, belowY };
        std::copy(std::begin(xOrder), std::end(xOrder), candidateX);
        std::copy(std::begin(yOrder), std::end(yOrder), candidateY);
    }

    int labelX = 2;
    int labelY = 2;
    auto tryPlacement = [&](int proposedX, int proposedY) {
        const int trialX = std::max(2, std::min(SCREEN_SIZE - labelWidth - 2, proposedX));
        const int trialY = std::max(2, std::min(SCREEN_SIZE - labelHeight - 2, proposedY));
        for (int index = 0; index < labelBoxCount; ++index) {
            constexpr int padding = 1;
            if (trialX < labelBoxX[index] + labelBoxWidth[index] + padding &&
                trialX + labelWidth + padding > labelBoxX[index] &&
                trialY < labelBoxY[index] + labelBoxHeight[index] + padding &&
                trialY + labelHeight + padding > labelBoxY[index]) {
                return false;
            }
        }
        labelX = trialX;
        labelY = trialY;
        return true;
    };

    bool placed = false;
    for (size_t candidate = 0; candidate < sizeof(candidateX) / sizeof(candidateX[0]); ++candidate) {
        if (tryPlacement(candidateX[candidate], candidateY[candidate])) {
            placed = true;
            break;
        }
    }

    for (int distance = 10; !placed && distance <= 150; distance += 10) {
        const int xChoices[] = { centeredX, rightX, leftX };
        const int yChoices[] = { aboveY - distance, belowY + distance };
        for (const int trialY : yChoices) {
            for (const int trialX : xChoices) {
                if (tryPlacement(trialX, trialY)) {
                    placed = true;
                    break;
                }
            }
            if (placed)
                break;
        }
    }

    for (int trialY = 26; !placed && trialY < HORIZON_Y - labelHeight - 6; trialY += labelHeight + 2) {
        const int xChoices[] = { 2, (SCREEN_SIZE - labelWidth) / 2, SCREEN_SIZE - labelWidth - 2 };
        for (const int trialX : xChoices) {
            if (tryPlacement(trialX, trialY)) {
                placed = true;
                break;
            }
        }
    }

    if (!placed || labelBoxCount >= 24)
        return;

    labelBoxX[labelBoxCount] = labelX;
    labelBoxY[labelBoxCount] = labelY;
    labelBoxWidth[labelBoxCount] = labelWidth;
    labelBoxHeight[labelBoxCount] = labelHeight;
    ++labelBoxCount;

    backbuffer.fillRect(labelX - 1, labelY - 1, labelWidth + 2, labelHeight + 2, Rgb(0, 0, 8));
    backbuffer.drawString(labelText, labelX, labelY);
}
