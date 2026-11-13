# 温湿度显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

每两秒读取 SHTC3 温度和相对湿度，采用相同的单色卡片界面显示。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

使用 **ESP-IDF v6.0.2**（[环境说明](../../../README_ZH.md)），在当前示例目录运行：

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## 预期现象

屏幕显示摄氏温度和相对湿度，读取失败时显示 --.-。

## 常见问题

使用板载传感器及 BSP 的 I2C 总线，不需要外接传感器。
