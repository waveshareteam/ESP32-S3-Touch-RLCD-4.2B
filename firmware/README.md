# Firmware

[English](README.md) | [简体中文](README_ZH.md)

The factory firmware for ESP32-S3-Touch-RLCD-4.2B is a tested merged image that you can flash at `0x0`. The date in the filename identifies the version.

| File | Purpose |
| --- | --- |
| [ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261125.bin](ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261125.bin) | Factory firmware with 11 apps, including AIChats with **Hi ESP** wake word support |

Download the BIN and open a terminal in its directory. Connect the board using a USB data cable, then run:

```sh
python -m pip install esptool
python -m esptool --chip esp32s3 erase-flash
python -m esptool --chip esp32s3 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261125.bin
```

The target is ESP32-S3 with 16 MB Flash. After flashing, power the board on again and follow the [factory firmware user guide](../examples/factory/README.md) to try the apps.

Erasing clears saved Wi-Fi, volume, and activation information, which must be set up again. Files on the SD card are unaffected. The image includes the required speech model; user storage is blank, with no personal photos or saved Wi-Fi credentials.

The factory firmware source is in [examples/factory](../examples/factory/).
