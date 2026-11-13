# microSD read/write test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Mounts a FAT32 microSD card, writes and reads `/sdcard/test.txt`, compares the complete contents and then unmounts the card.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable. A FAT32 microSD card is required.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `04_sd_card_test.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

The screen and serial log report the result. Press RESET to run the test again.

## Troubleshooting

The test overwrites `/test.txt` on the card. Insert the card before boot; the example does not format cards automatically.
