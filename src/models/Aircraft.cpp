#include "models/Aircraft.h"

namespace JsonParser {
    template<>
    Aircraft Parse<Aircraft>(const JsonVariant& state) {
        constexpr float KNOTS_TO_METRES_PER_SECOND = 0.514444f;
        constexpr float FEET_TO_METRES = 0.3048f;
        constexpr float FEET_PER_MINUTE_TO_METRES_PER_SECOND = 0.00508f;

        Aircraft a{};
        a.icao24 = state["hex"] | "";
        a.callsign = state["flight"] | "";
        a.callsign.trim();
        a.originCountry = "";
        a.timePosition = 0;
        a.lastContact = 0;
        a.longitude = state["lon"] | 0.0f;
        a.latitude = state["lat"] | 0.0f;

        const JsonVariant altitude = state["alt_baro"];
        a.onGround = altitude.is<const char*>() && altitude.as<String>() == "ground";
        a.baroAltitude = altitude.is<float>() ? altitude.as<float>() * FEET_TO_METRES : 0.0f;
        a.geoAltitude = state["alt_geom"].is<float>()
            ? state["alt_geom"].as<float>() * FEET_TO_METRES
            : a.baroAltitude;
        a.velocity = (state["gs"] | 0.0f) * KNOTS_TO_METRES_PER_SECOND;
        a.trueTrack = state["track"] | 0.0f;
        a.verticalRate = (state["baro_rate"] | 0.0f) * FEET_PER_MINUTE_TO_METRES_PER_SECOND;
        a.squawk = state["squawk"] | "";
        a.spi = (state["spi"] | 0) != 0;
        a.positionSource = 0;
        a.category = 0;
        return a;
    }
}
