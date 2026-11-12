# microSD read/write test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Mounts a FAT32 microSD card, writes and reads `/sdcard/test.txt`, compares the complete contents and then unmounts the card.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable. A FAT32 microSD card is required.

## How to Run

Use an ESP-IDF version listed in the [product README](../../../README.md), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

The screen and serial log report the result. Press RESET to run the test again.

## Troubleshooting

The test overwrites `/test.txt` on the card. Insert the card before boot; the example does not format cards automatically.
