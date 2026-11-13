# Factory application

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Runs the 400 x 300 monochrome application desktop with drawing, games, weather, photos, sensors, settings, music, voice chat and hardware test applications.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B, a USB data cable and compatible speaker; a FAT32 microSD card is required for SD-based photos and music.

## How to Run

Use ESP-IDF 6.0.2 and run `idf.py build flash monitor` from this directory.

## Expected Result

The monochrome desktop appears; tap an icon to open an application.

Display Test repeats full-screen black for 10 seconds and white for 5 seconds; double-tap anywhere to exit.

## Troubleshooting

Network applications require Internet access. The reflective display requires ambient light and has no backlight.
