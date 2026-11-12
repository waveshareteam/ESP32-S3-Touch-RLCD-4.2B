# Sensor — 400 × 300 RLCD

[English](README.md) | [简体中文](README_ZH.md)

采用确认后的日期时间 + 温湿度布局：上方居中显示大字号时间和日期，
下方并排显示温度、湿度卡片。单位跟随数值宽度排列，支持负温度和 100.0%。
界面为 LVGL 原生控件，黑白配色；桌面和标题栏图标使用用户提供的图片转换。

## 代码入口

- `Sensor.cpp`：与 Games 相同的 Brookesia APP 生命周期封装，在 `main/main.cpp` 注册。
- `sensor_init/sensor_init.c`：布局、返回按钮、LVGL 刷新定时器、数值格式和无效数据占位。
- `service/sensor_service.c/.h`：后台采样、线程安全快照及数据结构；不直接调用 LVGL。
- `assets/image_sensor_mono.c`：I1 图标数组，桌面 64 × 64、标题栏 32 × 32。
- BSP `esp32_s3_touch_rlcd_4_2.c`：RTC、温湿度初始化、读取及内部校验。
- BSP `include/bsp/esp32_s3_touch_rlcd_4_2.h`：统一管理日期类型和硬件函数接口；
  APP 通过 `bsp/esp-bsp.h` 引入，不再拆分传感器头文件和源文件。

## 硬件与采样

沿用 BSP 已建立的 I2C 总线，SDA = GPIO13，SCL = GPIO14，与触摸共用。

| 器件 | I2C 地址 | 数据 | 目标采样周期 |
| --- | --- | --- | --- |
| PCF85063A | 0x51 | 日期、时间 | 1 秒 |
| SHTC3 | 0x70 | 温度、湿度 | 5 秒 |

周期参照 C5 Sensor；实际间隔还包含 I2C 传输及转换耗时。
SHTC3 使用无时钟拉伸转换，等待转换期间
不占用 I2C 总线，并校验通信结果和 CRC。硬件地址和数据仍需上板确认。

APP 安装时不访问传感器。首次打开时创建采样任务并初始化，关闭或暂停时停止轮询；
进行中的 I2C 事务自然结束，其结果按会话编号丢弃。任务保留并休眠，后续打开复用。
这仅停止软件读取，不等于给全部器件断电。初始化失败的器件在重新打开或恢复 APP 时重试。
UI 每 100 ms 检查快照，只更新文字发生变化的标签，减少 RLCD 无效整帧刷新。

## BSP 接口和 RTC 约定

先使用已有的 `bsp_i2c_init()` 建立总线（正常 APP 启动流程已经完成），再调用：

```c
pcf_init();
shtc3_init();
bsp_sensor_read_environment(&temperature, &humidity);
bsp_sensor_read_datetime(&datetime);
```

初始化可重复调用；重新初始化前释放旧的驱动句柄，避免重复打开 APP 时泄漏。
读函数返回 `esp_err_t`。初始化失败的传感器显示 `--`；RTC 已有成功数据时，
后续读取失败保留原显示值。

RTC 按 C5 BSP 的工厂测试流程执行：`pcf85063a_init()` 后调用
`pcf85063a_set_time_date()` 写入 `Set_Time`（2026-04-22 23:59:00，dotw=1 原样沿用 C5），
再每秒调用 `pcf85063a_get_time_date()`。每次重新初始化都会重设这个测试时间，
包括重新打开 Sensor；这是与 C5 相同的测试行为，并非实时时间同步。
驱动为 C5 同版本 1.1.1，日期读写统一采用驱动的 1970 年基准（1970–2069），
不再使用额外的 OS 标志拦截、2000 年基准换算或编译时间补偿。
设置接口 `bsp_sensor_set_datetime()` 同样通过该驱动写入，weekday 为 0–6。

RTC 硬件配置与读写顺序对齐 C5，RLCD APP 继续使用后台快照和 LVGL 定时器更新界面；
暂停时停止采样、恢复时重新初始化，避免后台任务操作已删除的界面。

## 编译与验证

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。
