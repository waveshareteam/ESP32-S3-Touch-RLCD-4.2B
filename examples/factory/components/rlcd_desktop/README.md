# Desktop status bar

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Adds the monochrome desktop time, expression, Wi-Fi connection icon and voltage-based battery estimate. Application pages retain the full display area.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with the peripherals used by this application.

## How to Run

Build and flash the [factory application](../../README.md), then open the corresponding desktop icon.

## Expected Result

The desktop polls cached state and updates changed values; GPIO18 press counts switch the expression. The BSP samples GPIO4 every 100 ms, restores battery voltage with the x3 divider, and updates its cache from each 20-sample window with at least 16 valid readings, a trimmed mean and 1/8 exponential smoothing.

Battery percentage uses a clamped linear 3.0–4.12 V estimate, 12 mV hysteresis and at most one percentage point per 30 seconds; downward changes can follow the estimate immediately at filtered voltages of 3.3 V or below. The first result takes about two seconds; missing or stale results show `--%`. Per-window voltage, percentage and sample-count details use DEBUG logging and are hidden at the default INFO level; initialization and sampling warnings remain visible. Charging and load affect this estimate; battery presence and charging state are not detected.

## Troubleshooting

See the [Chinese usage guide](README_ZH.md) for the existing detailed component notes.
