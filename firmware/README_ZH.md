# 固件

[English](README.md) | [简体中文](README_ZH.md)

在 ESP32-S3-Touch-RLCD-4.2B 开发板上实测过、可直接烧录的固件。每个文件都是从 `0x0` 烧录的完整合并镜像，文件名中的日期即版本。更新时保留旧版本，新版本排在前面。

| 文件 | 用途 |
| --- | --- |
| [ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261113.bin](ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261113.bin) | 出厂固件，包含画板、语音聊天、天气、音乐、2048、相册、传感器、设置、按键测试、屏幕测试和麦克风测试 11 个应用 |

下载 BIN 后，在文件所在目录打开终端。连接开发板，使用支持数据传输的 USB 线，执行：

```sh
python -m pip install esptool
python -m esptool --chip esp32s3 erase-flash
python -m esptool --chip esp32s3 write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB 0x0 ESP32-S3-Touch-RLCD-4.2B_FactoryOnly_20261113.bin
```

芯片为 ESP32-S3，Flash 为 16 MB。烧录完成后重新上电，按照[出厂固件使用说明](../examples/factory/README_ZH.md)体验各应用。

擦除会清除设备中保存的 Wi-Fi、音量及激活相关信息，需要重新设置；不会删除 SD 卡里的文件。镜像包含所需的语音模型，用户存储分区保持空白，不包含个人图片或已保存的 Wi-Fi 配置。

出厂固件源码在 [examples/factory](../examples/factory/) 目录。
