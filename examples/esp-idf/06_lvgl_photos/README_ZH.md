# 左右滑动相册

[English](README.md) | [简体中文](README_ZH.md)

## 功能

内置双猫插画和山月湖景两张 400×300 黑白图片，两个框架使用相同的图片数组。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

使用 **ESP-IDF v6.0.2**（[环境说明](../../../README_ZH.md)），在当前示例目录运行：

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## 预期现象

左滑下一张，右滑上一张，首尾循环，底部显示图片序号。

## 常见问题

图片直接编译到程序中，无需 SD 卡、PNG 解码或上传，不自动轮播。
