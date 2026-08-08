from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FactoryDisplayPortTest(unittest.TestCase):
    def test_vendor_tftespi_setup_matches_factory_wiring(self):
        setup = (ROOT / "lib" / "TFT_eSPI" / "User_Setup.h").read_text(
            encoding="utf-8"
        )
        required = {
            "ST7789_2_DRIVER": None,
            "TFT_WIDTH": "240",
            "TFT_HEIGHT": "240",
            "TFT_MOSI": "5",
            "TFT_SCLK": "3",
            "TFT_DC": "2",
            "TFT_RST": "6",
            "TFT_BL": "1",
            "SPI_FREQUENCY": "27000000",
        }
        for name, value in required.items():
            if value is None:
                pattern = rf"^\s*#define\s+{name}(?:\s|$)"
            else:
                pattern = rf"^\s*#define\s+{name}\s+{value}(?:\s|$)"
            self.assertRegex(setup, re.compile(pattern, re.MULTILINE))
        self.assertNotRegex(setup, re.compile(r"^\s*#define\s+TFT_CS\b", re.MULTILINE))

    def test_graphics_adapter_uses_vendor_tftespi_not_lovyangfx(self):
        source = (ROOT / "include" / "LGFX.h").read_text(encoding="utf-8")
        self.assertIn("#include <TFT_eSPI.h>", source)
        self.assertIn("using LGFX_Sprite = TFT_eSprite", source)
        self.assertIn("analogWrite(1, 255 - brightness)", source)
        self.assertNotIn("LovyanGFX", source)
        self.assertNotIn("Panel_GC9A01", source)

    def test_main_does_not_drive_spi_clock_as_backlight(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertNotIn("pinMode(3, OUTPUT)", source)
        self.assertNotIn("digitalWrite(3, HIGH)", source)
        self.assertIn("tft.setBrightness(255)", source)


if __name__ == "__main__":
    unittest.main()
