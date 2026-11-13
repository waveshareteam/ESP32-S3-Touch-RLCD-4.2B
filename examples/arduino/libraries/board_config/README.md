# Board support

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shares the factory-derived display, touch, RTC, climate, SD and audio BSP across all Arduino examples. The UI helpers and hardware calls match the ESP-IDF examples.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with 16 MB Flash and 8 MB OPI PSRAM.

## How to Run

Install the bundled libraries and include `board_config.h` from a sketch. `rlcd_arduino_config.h` supplies the component configuration normally provided by ESP-IDF Kconfig.

## Expected Result

The examples use the same panel commands, monochrome mapping, touch processing and peripheral APIs as the ESP-IDF BSP.

## Troubleshooting

Do not initialize Arduino Wire or I2S on the BSP-owned buses. LVGL 9.5.0 and the bundled Espressif source adaptations are required.
