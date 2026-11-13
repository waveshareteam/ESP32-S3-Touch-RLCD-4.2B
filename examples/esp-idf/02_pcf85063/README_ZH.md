# RTC 时钟

[English](README.md) | [简体中文](README_ZH.md)

## 功能

每次启动将 PCF85063A 设为 2026-11-12 16:00:00，显示大号时钟和日期；联网同步后显示北京时间，断网切回持续走时的 RTC，网络时间不回写 RTC。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

使用 **ESP-IDF v6.0.2**（[环境说明](../../../README_ZH.md)），在当前示例目录运行：

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## 预期现象

日期显示在大号时钟上方，冒号每秒闪烁；自动重连，持续离线时不重复打印断网日志。

## 常见问题

Wi-Fi 固定为 `weather-wifi` / `12345678`，在 ESP-IDF 的 `main/main.c` 或 Arduino 的 `src/main.c` 中修改；SNTP 成功前持续显示 RTC 时间。
