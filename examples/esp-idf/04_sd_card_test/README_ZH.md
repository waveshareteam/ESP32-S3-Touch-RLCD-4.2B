# microSD 读写测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

挂载 FAT32 microSD 卡，写入、读取并校验 `/sdcard/test.txt`，完成后卸载卡。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。需要 FAT32 microSD 卡。

## 怎么运行

使用[产品说明](../../../README_ZH.md)列出的 ESP-IDF 版本，在当前示例目录运行：

```sh
idf.py set-target esp32s3
idf.py build flash monitor
```

## 预期现象

屏幕和串口显示测试结果，按 RESET 可重新测试。

## 常见问题

测试会覆盖卡上的 `/test.txt`；上电前插入存储卡，示例不会自动格式化卡。
