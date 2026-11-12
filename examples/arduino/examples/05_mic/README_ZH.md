# 双麦克风录放测试

[English](README.md) | [简体中文](README_ZH.md)

## 功能

显示 MIC1/MIC2 实时电平，将双声道录音暂存在 PSRAM 中，最长五秒，可分别试听；播放音量为 100%，PCM 提升两倍并限幅，原始录音保持不变。

## 硬件需求

ESP32-S3-Touch-RLCD-4.2B 开发板。开发板和 USB 数据线。试听时连接匹配的扬声器。

## 怎么运行

安装 Arduino-ESP32 **3.3.12**，将项目文件夹位置设为 `examples/arduino`，或将其中的 `libraries` 复制到当前项目文件夹。打开 `05_mic.ino`，按以下设置上传。

| 工具菜单 | 取值 |
| --- | --- |
| 开发板 | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| PSRAM | OPI PSRAM |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| USB CDC On Boot | Enabled |

## 预期现象

点击 Record 录音后，可分别点击 MIC1、MIC2 试听；Stop 提前停止录放，Retry 重试音频初始化。

## 常见问题

复位后录音丢失，新录音覆盖旧录音；MIC3 为回声参考，不作为测试麦克风。
