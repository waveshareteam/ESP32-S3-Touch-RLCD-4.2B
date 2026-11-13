# LVGL touch demo

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Creates the same 400 x 300 monochrome LVGL 9 screen as the ESP-IDF/Arduino counterpart, with a high-contrast button and touch counter.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Use **ESP-IDF v6.0.2** (see the [product README](../../../README.md)), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

Tap the button to increment the counter.

## Troubleshooting

The reflective display requires ambient light and has no backlight.
