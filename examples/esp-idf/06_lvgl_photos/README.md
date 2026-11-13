# Swipe photo gallery

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Embeds two 400 x 300 black-and-white illustrations: two cats and a moonlit mountain lake. The image arrays are identical in both frameworks.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Use **ESP-IDF v6.0.2** (see the [product README](../../../README.md)), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

Swipe left for the next image and right for the previous one. Both ends wrap and the footer shows the current image number.

## Troubleshooting

Images are compiled into the application; no SD card, PNG decoder or upload is needed. There is no automatic slideshow.
