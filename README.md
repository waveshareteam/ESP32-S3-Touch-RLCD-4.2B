<div align="center">
  <h1>ESP32-S3-Touch-RLCD-4.2B</h1>
  <p><strong>400 x 300 monochrome reflective touch display development board</strong></p>
  <p><strong>English</strong> | <a href="README_ZH.md">简体中文</a></p>
  <p>
    <a href="https://github.com/waveshareteam/ESP32-S3-Touch-RLCD-4.2B/actions/workflows/build.yml"><img alt="Build" src="https://github.com/waveshareteam/ESP32-S3-Touch-RLCD-4.2B/actions/workflows/build.yml/badge.svg"></a>
    <a href="https://github.com/waveshareteam/ESP32-S3-Touch-RLCD-4.2B/releases/latest"><img alt="Latest Release" src="https://img.shields.io/github/v/release/waveshareteam/ESP32-S3-Touch-RLCD-4.2B"></a>
    <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/waveshareteam/ESP32-S3-Touch-RLCD-4.2B"></a>
  </p>
  <p>
    <a href="https://www.waveshare.com/">🌐 Product</a> &middot;
    <a href="firmware/">📦 Firmware</a> &middot;
    <a href="examples/esp-idf/">🧩 ESP-IDF</a> &middot;
    <a href="examples/arduino/examples/">🔧 Arduino</a> &middot;
    <a href="examples/factory/README.md">📚 Documentation</a>
  </p>
</div>

---

## ✨ Overview

This repository provides matching ESP-IDF and Arduino examples 01–06, plus the factory application source. The reflective display is black and white and has no backlight.

| Feature | Device / interface |
| --- | --- |
| MCU | ESP32-S3 |
| Memory | 16 MB Flash, 8 MB OPI PSRAM |
| Display | 4.2-inch, 400 x 300, ST7305, SPI |
| Touch | CST816, I2C |
| RTC / climate sensor | PCF85063A / SHTC3 |
| Audio | ES8311, ES7210, dual-microphone recording/playback |
| Storage | microSD, SDMMC 1-bit |

## 📦 Quick Start

1. Download the complete factory firmware with `FactoryOnly` in its filename from [firmware](firmware/).
2. Connect the board with a USB data cable, then follow the [flashing instructions](firmware/README.md) to erase and flash at `0x0`.
3. Power on again, view the desktop under ambient light, and tap an app icon.
4. Follow the [factory firmware user guide](examples/factory/README.md) to set up Wi-Fi, prepare an SD card, and try each app.

Factory application source is in [examples/factory](examples/factory/). To modify the examples, choose one below and follow its README to build and flash.

## 🧪 Examples

### ESP-IDF

| Example | Feature |
| --- | --- |
| [01_lvgl](examples/esp-idf/01_lvgl/) | LVGL touch demo |
| [02_pcf85063](examples/esp-idf/02_pcf85063/) | RTC clock |
| [03_shtc3](examples/esp-idf/03_shtc3/) | Temperature and humidity |
| [04_sd_card_test](examples/esp-idf/04_sd_card_test/) | microSD read/write test |
| [05_mic](examples/esp-idf/05_mic/) | Dual-microphone recording |
| [06_lvgl_photos](examples/esp-idf/06_lvgl_photos/) | Swipe photo gallery |

### Arduino

| Example | Feature |
| --- | --- |
| [01_lvgl](examples/arduino/examples/01_lvgl/) | LVGL touch demo |
| [02_pcf85063](examples/arduino/examples/02_pcf85063/) | RTC clock |
| [03_shtc3](examples/arduino/examples/03_shtc3/) | Temperature and humidity |
| [04_sd_card_test](examples/arduino/examples/04_sd_card_test/) | microSD read/write test |
| [05_mic](examples/arduino/examples/05_mic/) | Dual-microphone recording |
| [06_lvgl_photos](examples/arduino/examples/06_lvgl_photos/) | Swipe photo gallery |

Arduino dependencies are bundled in [examples/arduino/libraries](examples/arduino/libraries/). All sketches share `board_config` and LVGL 9.5.0.

## 🛠️ Development Environment

| Framework | Version | Examples |
| --- | --- | ---: |
| ESP-IDF | v6.0.2 | 6 |
| Arduino-ESP32 | 3.3.12 | 6 |
| LVGL | 9.5.0 | Both frameworks |

Versions and board options are recorded in [.github/product.json](.github/product.json).

## 📚 Resources

- [Schematic](hardware/schematic/)
- [Dimensions and 3D model](hardware/mechanical/)
- [Official product website](https://www.waveshare.com/)
- [ESP-IDF examples](examples/esp-idf/)
- [Arduino examples](examples/arduino/examples/)

## 🤝 Support

For product and usage questions, contact [Waveshare support](https://service.waveshare.com/).

## 📄 License

First-party examples use Apache License 2.0; see [LICENSE](LICENSE). Third-party libraries and fonts retain their respective licenses.
