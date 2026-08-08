from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class OfflineStarMapTests(unittest.TestCase):
    def read(self, relative_path):
        return (ROOT / relative_path).read_text(encoding="utf-8-sig")

    def test_main_disables_wifi_and_uses_serial_time(self):
        source = self.read("src/main.cpp")
        self.assertIn("WiFi.mode(WIFI_OFF)", source)
        self.assertIn("SerialTimeSync", source)
        self.assertNotIn("wm.autoConnect", source)

    def test_serial_protocol_sets_system_time(self):
        source = self.read("src/SerialTimeSync.cpp")
        self.assertIn("SETTIME %lld %d", source)
        self.assertIn("settimeofday", source)
        self.assertIn("TIME_OK", source)

    def test_star_map_calculates_required_bodies(self):
        source = self.read("src/StarMap.cpp")
        for body in ("BODY_SUN", "BODY_MOON", "BODY_MARS", "BODY_JUPITER", "BODY_SATURN"):
            self.assertIn(body, source)
        self.assertIn("Astronomy_Equator", source)
        self.assertIn("Astronomy_Horizon", source)
        self.assertIn("Astronomy_MoonPhase", source)

    def test_star_map_uses_full_screen_labels(self):
        source = self.read("src/StarMap.cpp")
        header = self.read("src/StarMap.h")
        self.assertIn("constexpr int HORIZON_Y = 180;", source)
        self.assertIn("constexpr int SKY_UPPER_HEIGHT = 150;", source)
        self.assertIn("constexpr int SKY_LOWER_HEIGHT = 47;", source)
        self.assertIn("signedAltitude = std::max(-90.0", source)
        self.assertIn("CHART_MARGIN + (360.0 - normalizedAzimuth) * AZIMUTH_SPAN / 360.0", source)
        self.assertNotIn("altitudeRings", source)
        self.assertNotIn("DOME_", source)
        self.assertIn('"(A%03.0f H%+03.0f)"', source)
        self.assertIn('"(A%03.0f H%+03.0f %2.0f%%)"', source)
        self.assertIn('"(A%03.0f H%+03.0f M%+.1f)"', source)
        self.assertIn("labelBoxCount = 2;", source)
        self.assertIn("candidateX[8]", source)
        self.assertIn("int BaseRadius(astro_body_t body)", source)
        self.assertIn("backbuffer.drawEllipse(x, y, markerExtent", source)
        self.assertIn('strftime(timeText, sizeof(timeText), "%H:%M:%S"', source)
        self.assertIn("drawString(timeText, 4, 3)", source)
        self.assertIn('drawCentreString("360", 8, HORIZON_Y + 20, 1)', source)
        self.assertIn('drawCentreString("270", 64, HORIZON_Y + 20, 1)', source)
        self.assertIn('drawCentreString("180", 120, HORIZON_Y + 20, 1)', source)
        self.assertIn('drawCentreString("90", 175, HORIZON_Y + 20, 1)', source)
        self.assertIn('drawCentreString("0", 231, HORIZON_Y + 20, 1)', source)
        self.assertIn("drawLine(x - 3, HORIZON_Y, x + 3, HORIZON_Y, object.color)", source)
        self.assertIn("fillCircle(x, y, radius + 2, Rgb(0, 0, 8))", source)
        self.assertIn("const bool crossesHorizon", source)
        self.assertIn("if (!crossesHorizon)", source)
        self.assertIn("VisibilityFactor(object.altitude)", source)
        self.assertNotIn("guideColor", source)
        self.assertNotIn("guideStart", source)
        self.assertIn("const int centeredX = x - labelWidth / 2;", source)
        self.assertIn("const int aboveY = y - radius - labelHeight - gap;", source)
        self.assertIn('snprintf(labelText, sizeof(labelText), "%s %s"', source)
        self.assertIn("labelY = 2;", source)
        self.assertIn("backbuffer.drawString(labelText, labelX, labelY);", source)
        self.assertIn("if (object.body == BODY_SUN)", source)
        self.assertIn("else if (object.body == BODY_MARS || object.body == BODY_JUPITER", source)
        self.assertIn("const int yOrder[] = { aboveY, aboveY, aboveY, belowY", source)
        self.assertIn("const int extraBelowY = y + radius + labelHeight + 4;", source)
        self.assertNotIn("backbuffer.drawString(details", source)
        self.assertIn("constexpr int CHART_MARGIN = 8;", source)
        self.assertIn("labelBoxX[16]", header)
        self.assertIn("labelBoxX[labelBoxCount] = objectX - markerExtent - 2;", source)
        self.assertIn("void StarMap::DrawMarker", source)
        self.assertIn("DrawMarker(object);", source)
        self.assertNotIn("DrawInformation", source)
        self.assertNotIn("DrawInformation", header)
    def test_horizon_transition_and_night_sky(self):
        source = self.read("src/StarMap.cpp")
        header = self.read("src/StarMap.h")
        self.assertIn("HORIZON_TRANSITION_DEGREES = 4.0", source)
        self.assertIn("object.altitude <= 0.0", source)
        self.assertIn("object.altitude < LABEL_APPEAR_DEGREES", source)
        self.assertIn("linear * linear * (3.0 - 2.0 * linear)", source)
        self.assertIn("BODY_JUPITER", source)
        self.assertIn("BODY_SATURN", source)
        self.assertIn("backbuffer.fillEllipse", source)
        self.assertIn("backbuffer.drawEllipse", source)
        self.assertIn("void StarMap::DrawNightStars()", source)
        self.assertIn("objects[0].altitude >= 0.0", source)
        self.assertIn("NIGHT_STARS", source)
        self.assertIn("backbuffer.setTextSize(2)", source)
        self.assertIn("labelBoxWidth[1] = 106", source)
        self.assertIn("SkyObject objects[5]", header)
        self.assertNotIn("DrawSkyBackground", source)
        self.assertNotIn("DrawDayDecor", source)
        self.assertNotIn("DAY_PARTICLES", source)
    def test_network_radar_sources_are_excluded(self):
        config = self.read("platformio.ini")
        for source_file in (
            "AircraftManager.cpp",
            "ConfigurationWebServer.cpp",
            "HttpRequestManager.cpp",
            "OpenSkyAuthTokenHandler.cpp",
        ):
            self.assertIn(f"-<{source_file}>", config)

    def test_windows_sync_tool_autodetects_and_sends_epoch(self):
        source = self.read("tools/StarTimeSync.ps1")
        self.assertIn("GetPortNames", source)
        self.assertIn("ToUnixTimeSeconds", source)
        self.assertIn('WriteLine("SETTIME $epoch $offsetMinutes")', source)


    def test_auto_sync_watches_usb_and_installs_at_login(self):
        watcher = self.read("tools/StarTimeAutoSync.ps1")
        installer = self.read("tools/Install-StarTimeAutoSync.ps1")
        self.assertIn("GetPortNames", watcher)
        self.assertIn("StarTimeSync.ps1", watcher)
        self.assertIn("MicroStarAutoTimeSync", watcher)
        self.assertIn("disconnected", watcher)
        self.assertIn("synchronized successfully", watcher)
        self.assertIn('GetFolderPath("Startup")', installer)
        self.assertIn("MicroStarAutoTimeSync.cmd", installer)
        self.assertIn("-WindowStyle", installer)

if __name__ == "__main__":
    unittest.main()
