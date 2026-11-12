# LVGL 触摸示例

[English](README.md) | [简体中文](README_ZH.md)

## 功能

显示 400×300 单色 LVGL 9 界面，包含高对比度按钮和触摸计数，与另一框架的示例保持一致。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

使用[产品说明](../../../README_ZH.md)列出的 ESP-IDF 版本，在当前示例目录运行：

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## 预期现象

轻点按钮，触摸计数递增。

## 常见问题

反射式屏幕没有背光，需要在环境光下观察。
