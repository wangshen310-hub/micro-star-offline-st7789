# Micro Star — ESP32-C3 Offline Star Map

![Concept render 1](assets/micro-star-effect-1.png)


An offline, open-source desktop star map for an ESP32-C3 and a 1.54-inch ST7789 display. It calculates the apparent positions of the Sun, Moon, and all seven observable planets other than Earth—Mercury, Venus, Mars, Jupiter, Saturn, Uranus, and Neptune—and renders them against a right-to-left azimuth horizon. Saturn is drawn with rings; objects below the horizon are hidden; rising and setting use a short smooth transition.

## Project and features

This is a hardware-specific port of [AnthonySturdy/micro-radar](https://github.com/AnthonySturdy/micro-radar), an ESP32-C3 desktop flight-radar project. The port retains the upstream project structure, display-adapter workflow, and MIT licence, then adds an offline astronomical display for the tested 12-pin panel.

- Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, and Neptune with azimuth, altitude, and detail fields.
- Saturn marker includes a ring; Mars retains its `M+1.3`-style magnitude field.
- Right-to-left scale: `0/N → 90/E → 180/S → 270/W → 360/N`.
- Horizon at about three-quarters of the 240×240 display.
- Below-horizon bodies are hidden; a 0–4° band animates rising/setting smoothly.
- Collision-checked labels, enlarged upper-left time, and fixed night-only star points.
- Wi-Fi and API access are disabled. Windows sends time over USB serial; calculations run on-device.

## Hardware specification

| Item | Verified configuration |
|---|---|
| MCU | ESP32-C3, single-core 32-bit RISC-V, up to 160 MHz |
| Memory | 320 KB SRAM; 4 MB flash target (`esp32-c3-devkitm-1`) |
| Wireless | 2.4 GHz 802.11 b/g/n Wi-Fi and Bluetooth LE 5 capability; disabled in this firmware |
| USB | Native USB Serial/JTAG for flashing and time synchronisation |
| Display | 1.54-inch, 240×240 TFT, ST7789/ST7789_2 controller |
| Panel | Soldered 12-pin SPI display; no touch controller in this port |
| Logic/power | 3.3 V logic; follow the board power design |
| Build | PlatformIO + Arduino-ESP32, C++17 |

### Tested display wiring

These pins come from `lib/TFT_eSPI/User_Setup.h` and `include/LGFX.h`; they are not a generic ESP32 wiring rule.

| Signal | GPIO |
|---|---:|
| TFT MOSI / SDA | GPIO5 |
| TFT SCLK / SCL | GPIO3 |
| TFT DC | GPIO2 |
| TFT RESET | GPIO6 |
| TFT backlight | GPIO1 |
| TFT CS | Tied low on the tested board; no GPIO definition |
| VCC / GND | 3.3 V / GND |

Confirm silk labels and cable direction before using another 12-pin display. Two panels both sold as “ST7789” can still differ in reset, backlight, colour order, or offsets.

## Build, flash, and time sync

```text
pio run -e esp32-c3-devkitm-1
pio run -e esp32-c3-devkitm-1 -t upload --upload-port COM3
```

The [`firmware/`](firmware/) folder contains a complete 4 MB recovery image (`micro-star-full-4MB.bin`, address `0x0`), application image (`micro-star-firmware.bin`, address `0x10000`), bootloader, partitions, `boot_app0`, and `SHA256SUMS.txt`.

The device has no battery-backed RTC. On Windows, run `tools/Install-StarTimeAutoSync.cmd` once. The login watcher detects `MICROSTAR` and sends:

```text
SETTIME <unix_seconds> <timezone_offset_minutes>
```

The device replies `TIME_OK`. `tools/StarTimeSync.cmd` is the manual fallback.

## Tests

```text
python -m unittest discover -s tests -v
```

## Attribution and licence

Derived from [AnthonySturdy/micro-radar](https://github.com/AnthonySturdy/micro-radar). Keep the upstream attribution and [MIT License](LICENSE). Astronomical calculations use the bundled [Astronomy Engine](https://github.com/cosinekitty/astronomy) library.


### First-time setup

Before the first run, enter the device location's latitude and longitude. These coordinates are required for local horizon and azimuth calculations; without them, the displayed celestial positions cannot be correct. Use decimal degrees, for example 31.2304, 121.4737.
On an unconfigured device, the screen shows LOCATION REQUIRED. Connect USB serial and send SETLOC <latitude> <longitude>, for example SETLOC 31.2304 121.4737. The device replies LOCATION_OK and stores the coordinates in flash. You can also run 	ools/StarLocationSetup.cmd.
