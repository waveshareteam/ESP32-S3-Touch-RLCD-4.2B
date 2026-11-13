# ST7305 反射式液晶驱动

[English](README.md) | [简体中文](README_ZH.md)

基于 ESP-IDF `esp_lcd` 的 ST7305 SPI 面板驱动，采用本工程出厂应用的初始化命令和 1bpp 帧缓冲映射。由 BSP 负责总线和面板配置，应用通过 BSP 显示接口调用。
