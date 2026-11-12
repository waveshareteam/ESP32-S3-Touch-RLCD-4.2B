# microSD 读写测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

挂载 FAT32 microSD 卡，写入、读取并校验 `/sdcard/test.txt`，完成后卸载卡。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。需要 FAT32 microSD 卡。

## 怎么运行

安装 Arduino-ESP32 **3.3.12**，将项目文件夹位置设为 `examples/arduino`，或将其中的 `libraries` 复制到当前项目文件夹。打开 `04_sd_card_test.ino`，按以下设置上传。

| 工具菜单 | 取值 |
| --- | --- |
| 开发板 | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## 预期现象

屏幕和串口显示测试结果，按 RESET 可重新测试。

## 常见问题

测试会覆盖卡上的 `/test.txt`；上电前插入存储卡，示例不会自动格式化卡。
