# 左右滑动相册

[English](README.md) | [简体中文](README_ZH.md)

## 功能

内置双猫插画和山月湖景两张 400×300 黑白图片，两个框架使用相同的图片数组。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

安装 Arduino-ESP32 **3.3.12**，将项目文件夹位置设为 `examples/arduino`，或将其中的 `libraries` 复制到当前项目文件夹。打开 `06_lvgl_photos.ino`，按以下设置上传。

| 工具菜单 | 取值 |
| --- | --- |
| 开发板 | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## 预期现象

左滑下一张，右滑上一张，首尾循环，底部显示图片序号。

## 常见问题

图片直接编译到程序中，无需 SD 卡、PNG 解码或上传，不自动轮播。
