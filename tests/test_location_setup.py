from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class LocationSetupTests(unittest.TestCase):
    def read(self, relative_path):
        return (ROOT / relative_path).read_text(encoding="utf-8-sig")

    def test_first_boot_requires_location_instead_of_using_a_default(self):
        source = self.read("src/main.cpp")
        self.assertIn("LocationConfig", source)
        self.assertIn("LOCATION_REQUIRED", source)
        self.assertIn("locationConfig.Begin()", source)
        self.assertNotIn("DEFAULT_LATITUDE", source)
        self.assertNotIn("DEFAULT_LONGITUDE", source)

    def test_setloc_is_validated_and_persisted(self):
        source = self.read("src/LocationConfig.cpp")
        serial = self.read("src/SerialTimeSync.cpp")
        self.assertIn('"SETLOC %lf %lf"', source)
        self.assertIn('"LOCATION_OK "', source)
        self.assertIn('"LOCATION_ERROR RANGE"', source)
        self.assertIn('putString("latitude"', source)
        self.assertIn('putString("longitude"', source)
        self.assertIn("locationConfig->HandleCommand(line)", serial)

    def test_documents_explain_first_time_coordinates(self):
        english = self.read("README.en.md")
        chinese = self.read("README.zh-CN.md")
        self.assertIn("First-time setup", english)
        self.assertIn("SETLOC <latitude> <longitude>", english)
        self.assertIn("首次配置", chinese)
        self.assertIn("SETLOC <纬度> <经度>", chinese)


if __name__ == "__main__":
    unittest.main()
