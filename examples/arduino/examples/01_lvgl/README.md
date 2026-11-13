# LVGL touch demo

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Creates the same 400 x 300 monochrome LVGL 9 screen as the ESP-IDF/Arduino counterpart, with a high-contrast button and touch counter.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `01_lvgl.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

Tap the button to increment the counter.

## Troubleshooting

The reflective display requires ambient light and has no backlight.
