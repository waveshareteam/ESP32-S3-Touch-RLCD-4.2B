# Temperature and humidity

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Reads SHTC3 temperature and relative humidity every two seconds and displays both values in matching monochrome cards.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable.

## How to Run

Use **ESP-IDF v6.0.2** (see the [product README](../../../README.md)), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

The screen shows temperature in degrees Celsius and humidity in % RH. Failed readings appear as --.-.

## Troubleshooting

The onboard sensor uses the BSP I2C bus; no external sensor is required.
