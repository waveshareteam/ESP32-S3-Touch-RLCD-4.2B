# 板级支持

[English](README.md) | [简体中文](README_ZH.md)

## 功能

为全部 Arduino 示例提供同一套显示、触摸、RTC、温湿度、SD 与音频 BSP，沿用 ESP-IDF 的 UI 辅助函数和外设接口。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B，16 MB Flash、8 MB OPI PSRAM。

## 怎么运行

安装随附公共库，在草图中包含 `board_config.h`；`rlcd_arduino_config.h` 提供通常由 ESP-IDF Kconfig 生成的组件配置。

## 预期现象

各示例使用与 ESP-IDF BSP 相同的屏幕命令、单色映射、触摸处理和外设接口。

## 常见问题

不要在 BSP 占用的总线上重复初始化 Arduino Wire 或 I2S；需要 LVGL 9.5.0 及随附的 Espressif 组件源码适配库。
