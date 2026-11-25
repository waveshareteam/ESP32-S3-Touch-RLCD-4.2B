# Voice chat

[English](README.md) | [简体中文](README_ZH.md)

## What It Does

Provides activation and online voice conversation via **Hi ESP** or the **Talk** button. The BSP serializes audio access with Music and Mic Test.

## Hardware Required

ESP32-S3-Touch-RLCD-4.2B with the peripherals used by this application.

## How to Run

Build and flash the [factory application](../../README.md), then open the corresponding desktop icon.

## Expected Result

Connect the board to an Internet-enabled 2.4 GHz Wi-Fi network in Settings, then open AIChats. Activation and replies require the remote service.

To reply in the language spoken, configure the device's assistant with speech recognition and a voice that support English and Mandarin Chinese. In the assistant's role instructions, ask it to follow the language of the latest question. The local wake word is the same for both languages.

## Troubleshooting

See the [Chinese usage guide](README_ZH.md) for the existing detailed component notes.
