# Micro Star — ESP32-C3 离线星图

![效果图 1](assets/micro-star-effect-1.png)

![效果图 2](assets/micro-star-effect-2.png)

这是面向 ESP32-C3 与 1.54 英寸 ST7789 屏幕的离线星图固件，在设备本地计算太阳、月亮、火星、木星和土星的实时方位角与高度角，并显示从右向左的地平线刻度。土星带环；低于地平线的天体隐藏；升起和落下使用平滑过渡。

## 项目与功能

本项目移植自 [AnthonySturdy/micro-radar](https://github.com/AnthonySturdy/micro-radar)，原项目是 ESP32-C3 桌面飞机雷达。本移植保留其项目结构、显示适配工作流和 MIT 许可证，并加入针对实测 12 针 ST7789 屏幕的离线星图功能。

- 显示太阳、月亮、火星、木星和土星，以及方位角、高度角和附加信息。
- 土星符号带环；火星保留 `M+1.3` 形式的星等信息。
- 方位轴从右向左：`0/N → 90/E → 180/S → 270/W → 360/N`。
- 地平线位于屏幕约四分之三高度；低于地平线的天体隐藏。
- 穿越地平线时在 0–4° 范围内平滑出现或消失，标签整体防重叠。
- 左上角显示放大的时间，夜间显示固定星点。
- 固件关闭 Wi-Fi 和 API；Windows 通过 USB 串口发送时间，计算在设备本地完成。

## ESP32-C3 与屏幕规格

| 项目 | 已验证配置 |
|---|---|
| 主控 | ESP32-C3，单核 32 位 RISC-V，最高 160 MHz |
| 内存 | 320 KB SRAM；4 MB Flash |
| 无线 | 2.4 GHz Wi-Fi、Bluetooth LE 5；本固件关闭 |
| USB | 原生 USB Serial/JTAG，用于烧录和校时 |
| 显示屏 | 1.54 英寸、240×240 彩色 TFT，ST7789/ST7789_2 |
| 接口 | 焊接 12 针 SPI；本移植未使用触摸 |
| 逻辑/供电 | 3.3 V；遵循电路板供电设计 |
| 构建 | PlatformIO + Arduino-ESP32，C++17 |

### 实测屏幕引脚

以下引脚来自 `lib/TFT_eSPI/User_Setup.h` 和 `include/LGFX.h`，不代表所有 ESP32 的通用接法。

| 信号 | GPIO |
|---|---:|
| TFT MOSI / SDA | GPIO5 |
| TFT SCLK / SCL | GPIO3 |
| TFT DC | GPIO2 |
| TFT RESET | GPIO6 |
| TFT 背光 | GPIO1 |
| TFT CS | 实测板直接接地，没有 GPIO 定义 |
| VCC / GND | 3.3 V / GND |

更换其他 12 针屏幕前，必须确认丝印和排线方向。“ST7789”屏幕的复位、背光、颜色顺序和偏移量仍可能不同。

## 编译、烧录与校时

```text
pio run -e esp32-c3-devkitm-1
pio run -e esp32-c3-devkitm-1 -t upload --upload-port COM3
```

`firmware/` 目录提供完整 4 MB 恢复镜像 `micro-star-full-4MB.bin`（地址 `0x0`）、应用固件 `micro-star-firmware.bin`（地址 `0x10000`）、引导文件、分区表、`boot_app0` 和 `SHA256SUMS.txt`。

设备没有电池 RTC。Windows 首次运行 `tools/Install-StarTimeAutoSync.cmd` 后，登录任务会监听 `MICROSTAR` 并发送：

```text
SETTIME <Unix秒数> <时区偏移分钟>
```

设备返回 `TIME_OK`；`tools/StarTimeSync.cmd` 可手动校时。

## 测试

```text
python -m unittest discover -s tests -v
```

## 来源、致谢与许可证

本项目基于并移植扩展自 [AnthonySturdy/micro-radar](https://github.com/AnthonySturdy/micro-radar)。再次分发时请保留上游致谢和 [MIT License](LICENSE)。天文计算使用随项目提供的 [Astronomy Engine](https://github.com/cosinekitty/astronomy) 库。
