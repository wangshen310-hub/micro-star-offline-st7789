from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class AdsbLolPortTests(unittest.TestCase):
    def test_aircraft_manager_uses_adsb_lol_point_endpoint(self):
        source = (ROOT / "src" / "AircraftManager.cpp").read_text(encoding="utf-8-sig")
        self.assertIn("https://api.adsb.lol/v2/lat/", source)
        self.assertIn('doc["ac"]', source)
        self.assertNotIn("opensky-network.org/api/states/all", source)

    def test_parser_maps_adsb_lol_object_fields_and_units(self):
        source = (ROOT / "src" / "models" / "Aircraft.cpp").read_text(encoding="utf-8-sig")
        for field in ('state["hex"]', 'state["flight"]', 'state["lat"]', 'state["lon"]', 'state["gs"]', 'state["track"]'):
            self.assertIn(field, source)
        self.assertIn("KNOTS_TO_METRES_PER_SECOND", source)
        self.assertIn("FEET_TO_METRES", source)

    def test_http_manager_rejects_non_2xx_statuses(self):
        source = (ROOT / "src" / "HttpRequestManager.cpp").read_text(encoding="utf-8-sig")
        self.assertGreaterEqual(source.count("responseCode >= 200 && responseCode < 300"), 2)

    def test_configuration_page_no_longer_requests_opensky_credentials(self):
        source = (ROOT / "src" / "ConfigurationWebServer.cpp").read_text(encoding="utf-8-sig")
        self.assertNotIn("OpenSkyAPI Client", source)
        self.assertNotIn('TrySaveParam("opensky-id")', source)

if __name__ == "__main__":
    unittest.main()
