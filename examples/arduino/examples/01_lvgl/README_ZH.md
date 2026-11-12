# LVGL 触摸示例

[English](README.md) | [简体中文](README_ZH.md)

## 功能

显示 400×300 单色 LVGL 9 界面，包含高对比度按钮和触摸计数，与另一框架的示例保持一致。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

安装 Arduino-ESP32 **3.3.12**，将项目文件夹位置设为 `examples/arduino`，或将其中的 `libraries` 复制到当前项目文件夹。打开 `01_lvgl.ino`，按以下设置上传。

| 工具菜单 | 取值 |
| --- | --- |
| 开发板 | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## 预期现象

轻点按钮，触摸计数递增。

## 常见问题

反射式屏幕没有背光，需要在环境光下观察。
