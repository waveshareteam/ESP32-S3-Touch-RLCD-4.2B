# RTC clock

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Presets the PCF85063A to 2026-11-12 16:00:00 at boot and displays a large HH:MM clock with the date. Wi-Fi/SNTP selects Beijing time while online; disconnecting returns to the independently running RTC without writing network time back.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Use **ESP-IDF v6.0.2** (see the [product README](../../../README.md)), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

The date appears above the large clock; the colon blinks once per second. Reconnection is automatic and repeated offline failures are not logged.

## Troubleshooting

Wi-Fi is fixed to `weather-wifi` / `12345678` in `main/main.c` (ESP-IDF) or `src/main.c` (Arduino). Until SNTP replies successfully, the display continues to use RTC time.
