# RLCD Weather APP

[English](README.md) | [简体中文](README_ZH.md)

Weather 按 `Games` 的 Brookesia APP 结构封装，适配 400 × 300、LVGL 9.5 的 I1 黑白显示。UI 使用已确认的预览布局：顶部返回、标题和日期，中部城市、温度与天气图案，底部湿度、风速、风向。天气页面不包含配网入口。

## 手机热点直连与使用

1. 手机开启移动数据及个人热点，热点名称设为 `weather-wifi`，密码设为 `12345678`，选择 **2.4 GHz** 频段。iPhone 可开启“最大兼容性”。
2. 烧录当前固件并上电，设备自动以 STA 模式连接手机热点，无需扫码或在设备上填写密码。
3. 点击桌面 `Weather`。未连接时显示 `Connecting to weather-wifi...` 等状态，获取 IP 后自动请求天气并显示数据。
4. 手机热点可以在设备上电后再开启；后台每 10 秒尝试重连。断网恢复后自动重新获取天气，无需重启 APP。

本轮按原 C5 工程的预设热点逻辑测试，固定使用 `wifi_manager.h` 中的 `WIFI_DEFAULT_STA_SSID` / `WIFI_DEFAULT_STA_PASSWORD`。以前存入 NVS 的 Wi-Fi 配置保留，但不参与当前启动网络选择，也不需要擦除 NVS。45 秒未连接成功只更新等待提示并继续重试，不开启设备配网热点。

扫码及网络选择入口属于后续设置 APP，本轮未移植。天气页面的 Wi-Fi 按钮、二维码面板和自动弹窗已经移除。

- 点击天气图案：立即请求刷新。
- `Back`：关闭天气 APP 并回到桌面。
- 底部短促上滑：按现有系统手势返回桌面；再点 Weather 恢复页面。
- 自动更新：成功后间隔 5 分钟；请求失败后间隔 30 秒重试。
- 缓存：APP 关闭或断网后保留本次上电期间最后一次成功的数据，页面显示离线或错误状态。天气缓存不写入 NVS，重启后重新获取。

## 目录与接口

```text
Weather.hpp / Weather.cpp     单例、Brookesia 生命周期
weather_init/                UI 创建/销毁、暂停恢复、快照更新
ui/                          400x300 黑白天气布局与网络状态提示
custom/weather_model.*       原 C5 字段解析、英文显示、URL 编码
service/weather_service.*    后台 HTTP 请求、刷新调度、缓存
service/weather_body.*       有长度和超时限制的响应正文读取
assets/weather_icons.*       按天气状态切换的 I1 图案与接口声明
assets/image_weather_mono.c  用户提供的太阳云朵桌面图标，64x64 I1 C 数组
assets/image_weather_mono.png 与桌面图标 C 数组一致的黑白预览
assets/make_icons.c          天气状态图案生成工具，不生成桌面图标，不参与固件编译
../network_service/          手机热点直连；预留设置 APP 使用的配网服务
```

`Weather::requestInstance()` 与 `phone->installApp()` 的用法和 Games 一致。安装时 `init()` 启动天气后台任务；`run()` 在 Brookesia 当前 APP 屏幕上创建 UI；`close()` / `deinit()` 销毁 UI；`pause()` / `resume()` 暂停或恢复 UI 更新。网络与天气任务为共享服务，不随 APP 关闭而停止。

UI 层通过 `weather_service_get_snapshot()` 和 `wifi_manager_get_snapshot()` 读取数据，每 500 ms 检查一次，只有快照变化才更新控件。后台任务不操作 LVGL。原有 BSP、ST7305 刷新逻辑及手势配置不在本组件内修改。

保留原 C5 天气 UI 的七个字段名，后续替换布局时可继续对接：

| UI 字段 | 数据 |
| --- | --- |
| `screen_diqu_lab` | 城市显示名 |
| `screen_data_lab` | 接口更新时间的日期 |
| `screen_weather_lab` | 天气文字 |
| `screen_wendu_lab` | 温度 |
| `screen_shidu_lab` | 湿度 |
| `screen_fengsu_lab` | 风速 |
| `screen_fengxiang_lab` | 风向 |

## 与 C5 逻辑的对齐

继续使用原 C5 的天气接口与字段：`weather1`、`uptime`、`nowinfo.temperature`、`nowinfo.humidity`、`nowinfo.windDirection`、`nowinfo.windSpeed`。中文天气与风向转换为英文显示，避免依赖完整中文字库；天气图案按晴、多云、阴、雨、雷雨、雪、雾及未知状态选择。风速显示接口数值，不假定原接口未明确给出的单位。

启动时恢复原 C5 的 `weather-wifi` 手机热点直连行为，天气 APP 只读取网络状态。网络服务独立于 Weather，其他 APP 可使用 `wifi_manager_is_connected()` 或读取快照判断网络状态。

原有后台热点网页服务及 `POST /wifi_config`、`GET /status` 协议暂保留，只有明确调用 `wifi_manager_start_provisioning()` 才会启动，当前工程没有调用入口。该服务仍可将验证成功的凭据写入 `rlcd_wifi` / `credentials`；后续设置 APP 接入时再统一网络选择和启动策略。

这是行为与数据协议的对齐，并非直接复制 C5 的 Wi-Fi 实现。RLCD 的 `wifi_manager_init()` / `wifi_manager_start_provisioning()` 返回 `bool`，调用时按 `true` 成功判断。

## 天气 API 配置

默认沿用 C5 工程的 API ID、API Key，以及广东 / 深圳查询参数，屏幕显示 `Shenzhen`。在 `idf.py menuconfig` 中搜索并修改以下配置，再重新编译烧录：

- `RLCD_WEATHER_API_ID`、`RLCD_WEATHER_API_KEY`：原天气服务账户参数。
- `RLCD_WEATHER_PROVINCE`、`RLCD_WEATHER_CITY`：请求使用的省份和城市。
- `RLCD_WEATHER_CITY_LABEL`：屏幕显示名，使用现有字体支持的英文。

工程启用 Montserrat 48 大温度字体。天气图案和桌面图标已封装成 I1 C 数组，无需从文件系统加载图片。

## 编译与验证

在工程根目录、ESP-IDF 5.5.4 环境中执行：

```text
idf.py build
idf.py -p COMx flash monitor
```

`COMx` 换为实际串口。`build/esp32_s3_touch_rlcd_4_2b.bin` 是应用分区镜像；首次烧录请使用 `idf.py flash`，同时写入 bootloader 和分区表。

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。
