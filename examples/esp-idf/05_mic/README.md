# Dual-microphone recording

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Displays live MIC1/MIC2 meters, records both channels for up to five seconds in PSRAM and plays either microphone through the speaker. Playback uses volume 100% and a saturating 2x PCM gain; raw recordings are preserved.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable. Connect a compatible speaker for playback.

## How to Run

Use an ESP-IDF version listed in the [product README](../../../README.md), then run from this example directory:

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## Expected Result

Tap Record, speak, then tap MIC1 or MIC2 to listen. Stop ends a recording or playback early; Retry recovers an audio error.

## Troubleshooting

Recordings are lost on reset and replaced by the next recording. MIC3 is the echo reference and is excluded from the two microphone channels.
