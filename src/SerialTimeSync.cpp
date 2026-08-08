#include "SerialTimeSync.h"

#include <sys/time.h>
#include <time.h>

constexpr time_t MIN_VALID_EPOCH = 1704067200;  // 2024-01-01 UTC

void SerialTimeSync::Begin()
{
    input.reserve(96);
    Serial.println("MICROSTAR READY");
    Serial.println("TIME?");
}

void SerialTimeSync::Update()
{
    while (Serial.available() > 0) {
        const char ch = static_cast<char>(Serial.read());
        if (ch == '\n' || ch == '\r') {
            if (!input.isEmpty()) {
                HandleLine(input);
                input = "";
            }
        } else if (input.length() < 95) {
            input += ch;
        }
    }
}

bool SerialTimeSync::IsTimeValid() const
{
    return time(nullptr) >= MIN_VALID_EPOCH;
}

int SerialTimeSync::TimezoneOffsetMinutes() const
{
    return timezoneOffsetMinutes;
}

void SerialTimeSync::HandleLine(const String& rawLine)
{
    String line = rawLine;
    line.trim();

    if (line == "PING") {
        Serial.println("MICROSTAR");
        return;
    }

    if (line == "TIME?") {
        Serial.println(IsTimeValid() ? "TIME_VALID" : "TIME_REQUIRED");
        return;
    }

    long long epoch = 0;
    int offset = 480;
    if (sscanf(line.c_str(), "SETTIME %lld %d", &epoch, &offset) == 2) {
        if (epoch < MIN_VALID_EPOCH || offset < -720 || offset > 840) {
            Serial.println("TIME_ERROR RANGE");
            return;
        }

        timeval tv{};
        tv.tv_sec = static_cast<time_t>(epoch);
        tv.tv_usec = 0;
        if (settimeofday(&tv, nullptr) != 0) {
            Serial.println("TIME_ERROR SET");
            return;
        }

        timezoneOffsetMinutes = offset;
        Serial.print("TIME_OK ");
        Serial.print(static_cast<long long>(time(nullptr)));
        Serial.print(" ");
        Serial.println(timezoneOffsetMinutes);
        return;
    }

    Serial.println("TIME_ERROR COMMAND");
}
