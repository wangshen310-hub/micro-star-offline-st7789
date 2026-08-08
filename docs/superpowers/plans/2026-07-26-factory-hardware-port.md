# Factory Hardware Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Run the complete `micro-radar` application on the verified factory ESP32-C3 and soldered 12-pin ST7789 display while preserving a tested factory recovery path.

**Architecture:** Keep the existing application and LovyanGFX stack. Put all verified factory display values in one constants header, consume them from one LovyanGFX device class, and promote the firmware through a minimal colour diagnostic before uploading the full application.

**Tech Stack:** ESP32-C3, Arduino ESP32 2.0.17, PlatformIO, LovyanGFX, Python `unittest`, esptool, PowerShell.

## Global Constraints

- The working factory binary is the authoritative hardware reference.
- Use ST7789 with a 240 by 240 panel.
- Use MOSI GPIO5, SCLK GPIO3, DC GPIO2, RESET GPIO6, and active-low backlight GPIO1.
- Use no MISO, no busy pin, and no software-controlled CS.
- Start at 10 MHz SPI write frequency; 27 MHz is optional only after all runtime gates pass.
- Do not use the factory TFT_eSPI C3 register implementation.
- Do not erase the entire flash or intentionally erase NVS.
- Stop and restore the merchant images after a black screen, reboot loop, lost COM3, failed colour sequence, panic, watchdog reset, or Guru Meditation error.
- Preserve unrelated working-tree changes and stage only files named by each task.

---

## File Structure

- `include/FactoryDisplayConfig.h`: single source of truth for verified hardware constants.
- `include/LGFX.h`: LovyanGFX bus, panel, reset, and backlight wiring.
- `src/diagnostics/display_diagnostic.cpp`: no-network colour and heartbeat firmware.
- `src/main.cpp`: production application startup without manual ST7789 command injection.
- `platformio.ini`: production and diagnostic environments with mutually exclusive source filters.
- `tests/test_factory_display_port.py`: source-level regression tests for pin values, panel settings, diagnostic isolation, and production cleanup.
- `docs/factory-hardware-recovery.md`: exact factory restore image map and validation procedure.

---

### Task 1: Capture the verified working factory state

**Files:**
- Create outside repository: `../../outputs/factory-full-flash-verified-working-2026-07-26.bin`
- Create outside repository: `../../outputs/factory-nvs-verified-working-2026-07-26.bin`
- Modify outside repository: `../../outputs/BACKUP-README.txt`

**Interfaces:**
- Consumes: connected ESP32-C3 on `COM3`, 4 MB flash, merchant images.
- Produces: byte-for-byte recovery images and recorded SHA-256 hashes.

- [ ] **Step 1: Confirm the connected target without resetting the application**

Run:

```powershell
Get-CimInstance Win32_SerialPort | Where-Object DeviceID -eq 'COM3'
```

Expected: one `USB 串行设备 (COM3)` entry.

- [ ] **Step 2: Read the full 4 MB flash**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' '..\platformio-core\packages\tool-esptoolpy\esptool.py' --chip esp32c3 --port COM3 --baud 460800 read_flash 0x0 0x400000 '..\..\outputs\factory-full-flash-verified-working-2026-07-26.bin'
```

Expected: read completes and the file length is `4194304`.

- [ ] **Step 3: Read the factory NVS partition separately**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' '..\platformio-core\packages\tool-esptoolpy\esptool.py' --chip esp32c3 --port COM3 --baud 460800 read_flash 0x9000 0x5000 '..\..\outputs\factory-nvs-verified-working-2026-07-26.bin'
```

Expected: read completes and the file length is `20480`.

- [ ] **Step 4: Verify sizes and calculate hashes**

Run:

```powershell
Get-Item '..\..\outputs\factory-full-flash-verified-working-2026-07-26.bin','..\..\outputs\factory-nvs-verified-working-2026-07-26.bin' | Select-Object Name,Length
Get-FileHash -Algorithm SHA256 '..\..\outputs\factory-full-flash-verified-working-2026-07-26.bin','..\..\outputs\factory-nvs-verified-working-2026-07-26.bin'
```

Expected: lengths are `4194304` and `20480`; both hashes are recorded.

- [ ] **Step 5: Record the backup and merchant recovery map**

Append to `../../outputs/BACKUP-README.txt` the two filenames, lengths, SHA-256 hashes, and:

```text
Merchant restore map:
0x0000  SmallDesktopDisplay.ino.bootloader.bin
0x8000  SmallDesktopDisplay.ino.partitions.bin
0xe000  boot_app0.bin
0x10000 SmallDesktopDisplay.ino.bin
```

Expected: the recovery record contains no credentials and identifies the verified working backup.

---

### Task 2: Define factory display constants with a failing regression test

**Files:**
- Create: `tests/test_factory_display_port.py`
- Create: `include/FactoryDisplayConfig.h`

**Interfaces:**
- Consumes: verified pin and panel values from the design specification.
- Produces: `factory_display` constants used by `LGFX.h` and the diagnostic.

- [ ] **Step 1: Write the failing constants test**

Create `tests/test_factory_display_port.py`:

```python
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FactoryDisplayPortTests(unittest.TestCase):
    def test_verified_factory_constants(self):
        header = (ROOT / "include" / "FactoryDisplayConfig.h").read_text(
            encoding="utf-8"
        )
        expected = {
            "kPanelWidth": "240",
            "kPanelHeight": "240",
            "kMosiPin": "5",
            "kSclkPin": "3",
            "kDcPin": "2",
            "kResetPin": "6",
            "kBacklightPin": "1",
            "kChipSelectPin": "-1",
            "kMisoPin": "-1",
            "kDiagnosticSpiHz": "10000000",
        }
        for name, value in expected.items():
            self.assertRegex(
                header,
                rf"{name}\s*=\s*{re.escape(value)}\s*;",
                name,
            )
        self.assertRegex(header, r"kBacklightActiveLow\s*=\s*true\s*;")


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the test and verify the expected failure**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: error because `include/FactoryDisplayConfig.h` does not exist.

- [ ] **Step 3: Add the minimal constants header**

Create `include/FactoryDisplayConfig.h`:

```cpp
#pragma once

#include <cstdint>

namespace factory_display
{
inline constexpr int kPanelWidth = 240;
inline constexpr int kPanelHeight = 240;
inline constexpr int kMosiPin = 5;
inline constexpr int kSclkPin = 3;
inline constexpr int kDcPin = 2;
inline constexpr int kResetPin = 6;
inline constexpr int kBacklightPin = 1;
inline constexpr int kChipSelectPin = -1;
inline constexpr int kMisoPin = -1;
inline constexpr std::uint32_t kDiagnosticSpiHz = 10000000;
inline constexpr bool kBacklightActiveLow = true;
}
```

- [ ] **Step 4: Run the test and verify it passes**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: `test_verified_factory_constants` passes.

- [ ] **Step 5: Commit the constants and regression test**

Run:

```powershell
git add tests/test_factory_display_port.py include/FactoryDisplayConfig.h
git -c user.name=Codex -c user.email=codex@localhost commit -m "test: lock factory display wiring"
```

Expected: only the two named files are committed.

---

### Task 3: Build an isolated display diagnostic

**Files:**
- Modify: `tests/test_factory_display_port.py`
- Modify: `include/LGFX.h`
- Create: `src/diagnostics/display_diagnostic.cpp`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: constants in namespace `factory_display`.
- Produces: `LGFX` configured for the factory panel and PlatformIO environment `factory-display-diagnostic`.

- [ ] **Step 1: Add failing integration tests**

Add these methods to `FactoryDisplayPortTests`:

```python
    def test_lgfx_consumes_factory_constants(self):
        source = (ROOT / "include" / "LGFX.h").read_text(encoding="utf-8")
        required = [
            "Panel_ST7789",
            "factory_display::kMosiPin",
            "factory_display::kSclkPin",
            "factory_display::kDcPin",
            "factory_display::kResetPin",
            "factory_display::kChipSelectPin",
            "factory_display::kDiagnosticSpiHz",
            "factory_display::kBacklightPin",
            "factory_display::kBacklightActiveLow",
        ]
        for token in required:
            self.assertIn(token, source)
        self.assertNotRegex(source, r"pin_mosi\s*=\s*4\b")
        self.assertNotRegex(source, r"pin_rst\s*=\s*5\b")

    def test_diagnostic_is_network_free_and_has_all_colours(self):
        source = (
            ROOT / "src" / "diagnostics" / "display_diagnostic.cpp"
        ).read_text(encoding="utf-8")
        for label in ["RED", "GREEN", "BLUE", "WHITE", "BLACK"]:
            self.assertIn(f'"{label}"', source)
        self.assertIn("heartbeat=", source)
        self.assertNotIn("#include <WiFi", source)
        self.assertNotIn("WiFiManager", source)

    def test_platformio_separates_production_and_diagnostic_sources(self):
        source = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("[env:factory-display-diagnostic]", source)
        self.assertIn("-<diagnostics/>", source)
        self.assertIn("+<diagnostics/display_diagnostic.cpp>", source)
```

- [ ] **Step 2: Run the tests and verify the expected failures**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: the three new tests fail because integration has not been implemented.

- [ ] **Step 3: Replace the experimental `LGFX.h` configuration**

Implement `LGFX` with:

```cpp
#pragma once

#include <LovyanGFX.hpp>
#include "FactoryDisplayConfig.h"

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;

public:
    LGFX()
    {
        auto bus = _bus.config();
        bus.spi_host = SPI2_HOST;
        bus.spi_mode = 0;
        bus.freq_write = factory_display::kDiagnosticSpiHz;
        bus.freq_read = 8000000;
        bus.pin_mosi = factory_display::kMosiPin;
        bus.pin_miso = factory_display::kMisoPin;
        bus.pin_sclk = factory_display::kSclkPin;
        bus.pin_dc = factory_display::kDcPin;
        _bus.config(bus);
        _panel.setBus(&_bus);

        auto panel = _panel.config();
        panel.pin_cs = factory_display::kChipSelectPin;
        panel.pin_rst = factory_display::kResetPin;
        panel.pin_busy = -1;
        panel.memory_width = factory_display::kPanelWidth;
        panel.memory_height = factory_display::kPanelHeight;
        panel.panel_width = factory_display::kPanelWidth;
        panel.panel_height = factory_display::kPanelHeight;
        panel.offset_x = 0;
        panel.offset_y = 0;
        panel.offset_rotation = 0;
        panel.invert = true;
        panel.rgb_order = false;
        panel.readable = false;
        _panel.config(panel);

        auto light = _light.config();
        light.pin_bl = factory_display::kBacklightPin;
        light.invert = factory_display::kBacklightActiveLow;
        light.freq = 12000;
        light.pwm_channel = 7;
        _light.config(light);
        _panel.setLight(&_light);

        setPanel(&_panel);
    }
};
```

- [ ] **Step 4: Add the no-network diagnostic**

Create `src/diagnostics/display_diagnostic.cpp` with a five-entry table of
`RED`, `GREEN`, `BLUE`, `WHITE`, and `BLACK`, each using its corresponding
LovyanGFX colour constant. `setup()` initialises serial, calls `display.init()`,
sets brightness to 255, and prints the verified pins. `loop()` changes colour
every two seconds and prints `heartbeat=<counter>` once per second.

- [ ] **Step 5: Separate PlatformIO source sets**

Add to `[env:esp32-c3-devkitm-1]`:

```ini
build_src_filter =
    +<*>
    -<diagnostics/>
```

Add:

```ini
[env:factory-display-diagnostic]
extends = env:esp32-c3-devkitm-1
build_src_filter =
    -<*>
    +<diagnostics/display_diagnostic.cpp>
```

Remove the fixed macOS `monitor_port` and `upload_port` values from the base
environment so `COM3` is supplied explicitly by commands.

- [ ] **Step 6: Run source tests**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: all tests pass.

- [ ] **Step 7: Build the diagnostic environment**

Run:

```powershell
$env:PLATFORMIO_CORE_DIR = '..\platformio-core'
& '..\platformio-venv\Scripts\pio.exe' run -e factory-display-diagnostic
```

Expected: build succeeds for ESP32-C3 with no duplicate `setup()` or `loop()`.

- [ ] **Step 8: Commit the diagnostic implementation**

Run:

```powershell
git add tests/test_factory_display_port.py include/FactoryDisplayConfig.h include/LGFX.h src/diagnostics/display_diagnostic.cpp platformio.ini
git -c user.name=Codex -c user.email=codex@localhost commit -m "feat: add factory display diagnostic"
```

Expected: only the named diagnostic and hardware files are committed.

---

### Task 4: Run the diagnostic promotion gate

**Files:**
- Read: `.pio/build/factory-display-diagnostic/firmware.bin`
- Preserve: merchant recovery images and Task 1 backups.

**Interfaces:**
- Consumes: built diagnostic firmware and connected COM3 device.
- Produces: visible colour confirmation and 30 consecutive serial heartbeats.

- [ ] **Step 1: Upload only after confirming both backups exist**

Run:

```powershell
Get-Item '..\..\outputs\factory-full-flash-verified-working-2026-07-26.bin','..\..\outputs\factory-nvs-verified-working-2026-07-26.bin'
$env:PLATFORMIO_CORE_DIR = '..\platformio-core'
& '..\platformio-venv\Scripts\pio.exe' run -e factory-display-diagnostic --target upload --upload-port COM3
```

Expected: upload hash verification succeeds and COM3 returns.

- [ ] **Step 2: Capture at least 30 seconds of serial output**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' '..\read_serial_with_reset.py' --port COM3 --seconds 35
```

Expected: at least 30 increasing `heartbeat=` lines and no panic, watchdog,
Guru Meditation, or repeated boot banner.

- [ ] **Step 3: Ask the user to confirm the visible sequence**

Expected user observation: full-screen red, green, blue, white, and black,
each held for two seconds and then repeated.

- [ ] **Step 4: Stop on any gate failure**

If serial or display validation fails, restore the merchant images:

```powershell
& '..\platformio-venv\Scripts\python.exe' '..\platformio-core\packages\tool-esptoolpy\esptool.py' --chip esp32c3 --port COM3 --baud 460800 write_flash 0x0 'C:\Users\wangs\Desktop\烧录固件\时钟\C3版本\下载工具和说明\flash_download_tool_3.9.2_0\bin\SmallDesktopDisplay.ino.bootloader.bin' 0x8000 'C:\Users\wangs\Desktop\烧录固件\时钟\C3版本\下载工具和说明\flash_download_tool_3.9.2_0\bin\SmallDesktopDisplay.ino.partitions.bin' 0xe000 'C:\Users\wangs\Desktop\烧录固件\时钟\C3版本\下载工具和说明\flash_download_tool_3.9.2_0\bin\boot_app0.bin' 0x10000 'C:\Users\wangs\Desktop\烧录固件\时钟\C3版本\下载工具和说明\flash_download_tool_3.9.2_0\bin\SmallDesktopDisplay.ino.bin'
```

Expected after recovery: factory Wi-Fi configuration screen is visible.

---

### Task 5: Clean production startup and build the full application

**Files:**
- Modify: `tests/test_factory_display_port.py`
- Modify: `src/main.cpp`
- Read: all normal production sources selected by `platformio.ini`.

**Interfaces:**
- Consumes: validated `LGFX` class from Task 3.
- Produces: production startup with one LovyanGFX initialisation and no raw display commands.

- [ ] **Step 1: Add the failing production cleanup test**

Add:

```python
    def test_production_uses_single_lovyangfx_initialisation(self):
        source = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertEqual(source.count("tft.init();"), 1)
        self.assertIn("tft.setBrightness(255);", source)
        self.assertNotIn("applyFactorySt7789Init", source)
        self.assertNotIn("writecommand(", source)
        self.assertNotRegex(source, r"pinMode\s*\(\s*1\s*,")
        self.assertNotRegex(source, r"digitalWrite\s*\(\s*1\s*,")
```

- [ ] **Step 2: Run the test and verify failure**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: the new test fails because experimental raw commands and manual
backlight writes still exist.

- [ ] **Step 3: Remove experimental display code from `main.cpp`**

Delete `applyFactorySt7789Init`, all direct `writecommand`/`writedata` calls,
and all direct GPIO1 writes. Keep one:

```cpp
tft.init();
tft.setBrightness(255);
```

Keep the existing Wi-Fi, configuration server, OpenSky, aircraft update,
and rendering logic unchanged.

- [ ] **Step 4: Run all source tests**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
```

Expected: all tests pass.

- [ ] **Step 5: Build the production environment**

Run:

```powershell
$env:PLATFORMIO_CORE_DIR = '..\platformio-core'
& '..\platformio-venv\Scripts\pio.exe' run -e esp32-c3-devkitm-1
```

Expected: build succeeds and excludes `src/diagnostics/display_diagnostic.cpp`.

- [ ] **Step 6: Commit production cleanup**

Run:

```powershell
git add tests/test_factory_display_port.py src/main.cpp
git -c user.name=Codex -c user.email=codex@localhost commit -m "feat: use factory display in micro radar"
```

Expected: production cleanup and its regression test are committed.

---

### Task 6: Upload and verify the full micro-radar application

**Files:**
- Read: `.pio/build/esp32-c3-devkitm-1/firmware.bin`
- Preserve: all recovery images.

**Interfaces:**
- Consumes: production firmware from Task 5.
- Produces: visible micro-radar startup and stable configuration service.

- [ ] **Step 1: Upload the production environment**

Run:

```powershell
$env:PLATFORMIO_CORE_DIR = '..\platformio-core'
& '..\platformio-venv\Scripts\pio.exe' run -e esp32-c3-devkitm-1 --target upload --upload-port COM3
```

Expected: upload verification succeeds and COM3 returns.

- [ ] **Step 2: Monitor startup for 60 seconds**

Capture serial output for at least 60 seconds.

Expected: no panic, watchdog, Guru Meditation, or repeated boot banner.

- [ ] **Step 3: Verify the display and configuration path**

Expected:

- `Connecting to WiFi...` appears;
- if Wi-Fi is not stored, `MicroRadar-Setup` appears and its captive portal opens;
- if Wi-Fi is already stored, `http://microradar.local` is reachable.

- [ ] **Step 4: Configure and verify runtime**

Save Wi-Fi, location, radar radius, and OpenSky credentials through the
existing configuration page.

Expected: the radar UI renders and refreshes for five minutes without reset.

- [ ] **Step 5: Recover immediately on failure**

Use the exact merchant four-image command from Task 4, then verify the
factory Wi-Fi configuration screen before further investigation.

---

### Task 7: Document recovery and run final verification

**Files:**
- Create: `docs/factory-hardware-recovery.md`
- Verify: all files changed by Tasks 2, 3, and 5.

**Interfaces:**
- Consumes: verified backup hashes, merchant paths, final build and runtime results.
- Produces: repeatable recovery instructions and a final checked repository state.

- [ ] **Step 1: Write recovery documentation**

Document:

- verified pins and panel;
- dated full-flash and NVS backup names and hashes;
- merchant four-image address map;
- diagnostic and production build commands;
- serial and visual promotion criteria;
- recovery triggers.

- [ ] **Step 2: Run all tests and both builds**

Run:

```powershell
& '..\platformio-venv\Scripts\python.exe' -m unittest tests.test_factory_display_port -v
$env:PLATFORMIO_CORE_DIR = '..\platformio-core'
& '..\platformio-venv\Scripts\pio.exe' run -e factory-display-diagnostic
& '..\platformio-venv\Scripts\pio.exe' run -e esp32-c3-devkitm-1
```

Expected: all tests and both builds pass.

- [ ] **Step 3: Check repository hygiene**

Run:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors; only intentional files remain modified.

- [ ] **Step 4: Commit recovery documentation**

Run:

```powershell
git add docs/factory-hardware-recovery.md
git -c user.name=Codex -c user.email=codex@localhost commit -m "docs: add factory firmware recovery"
```

Expected: recovery instructions are committed separately.

