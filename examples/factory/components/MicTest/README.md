# Microphone test

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Shows MIC1/MIC2 digital RMS levels and clipping indicators, records up to five seconds in PSRAM and plays either microphone separately. MIC3 is the echo reference and is excluded.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with the peripherals used by this application.

## How to Run

Build and flash the [factory application](../../README.md), then open the corresponding desktop icon.

## Expected Result

Open Mic Test, tap Record and then MIC1 or MIC2 to listen. dBFS is an uncalibrated digital level, not sound pressure in dB SPL.

## Troubleshooting

See the [Chinese usage guide](README_ZH.md) for the existing detailed component notes.
