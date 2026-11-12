# Temperature and humidity

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads SHTC3 temperature and relative humidity every two seconds and displays both values in matching monochrome cards.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `03_shtc3.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

The screen shows temperature in degrees Celsius and humidity in % RH. Failed readings appear as --.-.

## Troubleshooting

The onboard sensor uses the BSP I2C bus; no external sensor is required.
