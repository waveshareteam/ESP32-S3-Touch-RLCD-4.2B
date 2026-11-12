# Desktop status bar

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Adds the monochrome desktop time, expression, Wi-Fi connection icon and voltage-based battery estimate. Application pages retain the full display area.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with the peripherals used by this application.

## How to Run

Build and flash the [factory application](../../README.md), then open the corresponding desktop icon.

## Expected Result

The desktop polls cached state and updates changed values; GPIO18 press counts switch the expression. Battery percentage is an estimate based on voltage.

## Troubleshooting

See the [Chinese usage guide](README_ZH.md) for the existing detailed component notes.
