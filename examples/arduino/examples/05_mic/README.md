# Dual-microphone recording

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Displays live MIC1/MIC2 meters, records both channels for up to five seconds in PSRAM and plays either microphone through the speaker. Playback uses volume 100% and a saturating 2x PCM gain; raw recordings are preserved.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B development board. The development board and a USB data cable. Connect a compatible speaker for playback.

## How to Run

Install Arduino-ESP32 **3.3.12** and set the sketchbook location to `examples/arduino`, or copy its `libraries` directory into your sketchbook. Open `05_mic.ino`, apply these settings and upload.

| Tools menu | Value |
| --- | --- |
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## Expected Result

Tap Record, speak, then tap MIC1 or MIC2 to listen. Stop ends a recording or playback early; Retry recovers an audio error.

## Troubleshooting

Recordings are lost on reset and replaced by the next recording. MIC3 is the echo reference and is excluded from the two microphone channels.
