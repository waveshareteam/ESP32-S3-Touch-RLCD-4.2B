# Settings

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provides volume control, Wi-Fi/network setup, SD card information and About pages. Saved volume is restored from NVS on reboot.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with the peripherals used by this application.

## How to Run

Build and flash the [factory application](../../README.md), then open the corresponding desktop icon.

## Expected Result

Use the desktop Settings icon. Volume changes apply immediately and are saved by the background worker when the control is released. SD storage is checked once in the background at each boot; the SD Card page shows the cached status, total space and free space without a manual check. Use Check card to refresh after inserting or replacing a card.

## Troubleshooting

See the [Chinese usage guide](README_ZH.md) for the existing detailed component notes.
