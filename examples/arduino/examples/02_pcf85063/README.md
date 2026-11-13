# RTC clock

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Presets the PCF85063A to 2026-11-12 16:00:00 at boot and displays a large HH:MM clock with the date. Wi-Fi/SNTP selects Beijing time while online; disconnecting returns to the independently running RTC without writing network time back.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `02_pcf85063.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

The date appears above the large clock; the colon blinks once per second. Reconnection is automatic and repeated offline failures are not logged.

## Troubleshooting

Wi-Fi is fixed to `weather-wifi` / `12345678` in `main/main.c` (ESP-IDF) or `src/main.c` (Arduino). Until SNTP replies successfully, the display continues to use RTC time.
