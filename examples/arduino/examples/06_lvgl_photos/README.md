# Swipe photo gallery

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Embeds two 400 x 300 black-and-white illustrations: two cats and a moonlit mountain lake. The image arrays are identical in both frameworks.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `06_lvgl_photos.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

Swipe left for the next image and right for the previous one. Both ends wrap and the footer shows the current image number.

## Troubleshooting

Images are compiled into the application; no SD card, PNG decoder or upload is needed. There is no automatic slideshow.
