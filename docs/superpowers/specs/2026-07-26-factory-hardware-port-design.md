# Micro Radar Factory Hardware Port Design

## Objective

Port `micro-radar` to the verified factory ESP32-C3 and soldered 12-pin
ST7789 display hardware without relying on the incompatible factory
TFT_eSPI C3 register implementation.

The port is successful only when:

- the display passes a red, green, blue, white, and black colour sequence;
- the firmware emits a continuing serial heartbeat without rebooting;
- the full application displays its Wi-Fi connection screen;
- the `MicroRadar-Setup` access point and configuration page are available;
- after configuration, the radar screen continues refreshing without crashes.

## Verified Hardware Baseline

The factory binary is the authoritative working reference. The bundled
factory `User_Setup.h` and the successful factory image establish:

- controller: ST7789 (`ST7789_2_DRIVER`);
- logical and panel size: 240 by 240 pixels;
- MOSI: GPIO5;
- SCLK: GPIO3;
- DC: GPIO2;
- RESET: GPIO6;
- backlight gate: GPIO1, active low;
- CS: permanently selected in hardware, so software uses `-1`;
- no MISO connection;
- factory SPI write frequency: 27 MHz.

The earlier GPIO4/RESET5 interpretation came from a conflicting schematic
and must not be used for this assembled board.

## Chosen Architecture

Retain the existing `micro-radar` application and LovyanGFX dependency.
Isolate all factory display details in the LovyanGFX hardware configuration.
Do not transplant the factory TFT_eSPI library and do not downgrade the
whole application to the factory Arduino core.

The first diagnostic build uses a 10 MHz SPI write frequency. After the
complete colour test succeeds, the full application may use 27 MHz. If
27 MHz causes instability, the final port remains at 10 MHz.

## Components

### Factory display configuration

`include/LGFX.h` owns the complete physical display configuration:

- `lgfx::Panel_ST7789`;
- `lgfx::Bus_SPI` on `SPI2_HOST`;
- the verified pins and active-low backlight;
- 240 by 240 panel and memory dimensions;
- no chip-select pin;
- no MISO or busy pin.

The hardware configuration is the only place where physical display pins
are defined.

### Diagnostic firmware

A separate PlatformIO environment builds a minimal diagnostic program. It:

1. starts serial output at 115200 baud;
2. enables the backlight on GPIO1;
3. initialises the display once through LovyanGFX;
4. displays red, green, blue, white, and black for two seconds each;
5. repeats the sequence and emits a numbered heartbeat once per second.

It does not start Wi-Fi, access NVS, or call the OpenSky API.

### Full application

The production environment builds the existing `micro-radar` application
with the same hardware configuration. Display initialisation happens once.
There is no extra hand-written ST7789 command sequence unless the diagnostic
test proves LovyanGFX's panel initialisation is insufficient.

## Firmware Safety and Recovery

Before any upload, read all 4 MB of the currently working device into a new
dated binary and calculate its SHA-256 hash. Retain:

- the new verified-factory full-flash backup;
- the merchant bootloader, partition table, `boot_app0`, and application
  images;
- the earlier micro-radar backups.

Do not erase the whole flash during either diagnostic or production upload.
Do not intentionally erase NVS. Upload only the normal PlatformIO build
artifacts required by the selected environment.

If the device enters a reboot loop, loses COM3, shows only backlight, or
fails the colour test, stop the migration. Restore the four merchant images
at `0x0`, `0x8000`, `0xe000`, and `0x10000`, then verify the factory Wi-Fi
configuration screen before investigating further.

## Test and Promotion Gates

### Gate 1: static configuration

The built diagnostic environment must report ESP32-C3, ST7789, 240 by 240,
MOSI 5, SCLK 3, DC 2, RESET 6, backlight 1 active low, and CS disabled.

### Gate 2: diagnostic runtime

The colour sequence must be visible and correctly coloured. Serial output
must show at least 30 consecutive heartbeat messages with no panic, Guru
Meditation, watchdog reset, or unexpected boot banner.

### Gate 3: production startup

The full build must show `Connecting to WiFi...`, expose
`MicroRadar-Setup` when unconfigured, and keep COM3 available without
rebooting for at least 60 seconds.

### Gate 4: application runtime

After Wi-Fi and OpenSky configuration, the screen must render the radar UI
and continue refreshing for at least five minutes without a reset.

Promotion to the next gate occurs only after the current gate passes.

## Non-goals

- Rewriting the micro-radar networking or radar logic.
- Reusing the factory clock UI.
- Repairing or modernising the bundled factory TFT_eSPI library.
- Optimising SPI performance before stable display operation is proven.
