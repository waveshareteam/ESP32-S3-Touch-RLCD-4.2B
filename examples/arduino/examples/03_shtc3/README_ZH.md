# 温湿度显示

[English](README.md) | [简体中文](README_ZH.md)

## 功能

每两秒读取 SHTC3 温度和相对湿度，采用相同的单色卡片界面显示。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。

## 怎么运行

安装 Arduino-ESP32 **3.3.12**，将项目文件夹位置设为 `examples/arduino`，或将其中的 `libraries` 复制到当前项目文件夹。打开 `03_shtc3.ino`，按以下设置上传。

| 工具菜单 | 取值 |
| --- | --- |
| 开发板 | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## 预期现象

屏幕显示摄氏温度和相对湿度，读取失败时显示 --.-。

## 常见问题

使用板载传感器及 BSP 的 I2C 总线，不需要外接传感器。
