# RLCD 桌面状态栏

[English](README.md) | [简体中文](README_ZH.md)

桌面状态栏位于屏幕顶部 `(0, 0)`，高 32 像素，采用黑底白字，放置系统时间、居中表情、Wi-Fi 状态和估算电量。APP 图标区位置保持不变，每页 4 列 × 2 排，图标为原生 64×64 的圆角黑底白色图案，点击区域为 100×126；第一页第一排为 Drawing、AIChats、Weather、Music，第二排为 2048、Photos、Sensor、Settings。第二页为 Button Test、Display Test、Mic Test，图标由 C5-3.5 原图转换为相同的黑底白色图案。AIChats 仅修改桌面显示名称，仍使用原 XiaozhiApp。

表情来自原 Settings 的黑白资源，离线缩小为 24×24 白色图案，居中放入 32×32 黑色位图，位图屏幕位置 `(184, 0)`。GPIO18 的初始化、低电平、消抖和按下计数继续由原 BSP 管理；默认微笑，每按一次切换微笑/闭嘴。设置页已移除表情和按键轮询。

时间和电量百分比使用 `assets/rlcd_font_status_16.c` 的紧凑 8×16、1-bit 点阵字体，字符为 `0123456789:-%`。字模取自 LVGL UNSCII 8，将每行离线重复两次以增加高度，运行时不缩放、不产生灰色抗锯齿边缘。五位时间宽 40 像素，`100%` 宽 32 像素，均可放入原控件。电池符号仍使用 Montserrat 16；Wi-Fi 图标统一显示为白色，APP 标签仍为白底黑字。

`rlcd_desktop_create()` 只挂载到 Brookesia 桌面容器，进入 APP 后随桌面隐藏，APP 保持完整 400×300。50 ms 的 LVGL 定时器只在桌面可见时读取内存中的按键计数和已有 Wi-Fi 连接状态，每秒读取一次系统时间和 BSP 电量缓存。仅变化时更新控件，分钟不变时不因时钟重绘，不增加后台任务。Wi-Fi 图标表示连接/断开，不测量信号强度。

联网校时由现有 `network_service/wifi_manager.c` 管理：STA 获得 IP 后，通过 ESP-NETIF 启动 SNTP，服务器为 `ntp.aliyun.com` 和 `pool.ntp.org`。启用两个服务器槽，联网期间按 SDK 配置每小时校时；断线重连后重新发起同步。初始化失败每 10 秒重试，等待网络回复由 lwIP 负责，不阻塞 UI。

系统时间保存 UTC 时间戳，时区使用 `CST-8`（北京时间 UTC+8）。本次启动尚未收到 SNTP 回复时，顶部显示 `--:--`；同步成功显示 `HH:MM`。断网后保留系统时钟继续走时，重新上电需再次联网校时。串口出现 `SNTP synced (UTC+8): ...` 才表示同步成功，`SNTP started` 只表示已启动请求。只连上无互联网的热点不会获得有效时间。

关闭 `CONFIG_XIAOZHI_SYNC_SYSTEM_TIME_FROM_SERVER`，避免小智的 OTA 时间覆盖 SNTP，并避免其时区偏移与本地时区叠加。桌面通过 `wifi_manager_get_local_time()` 读取时间，不读写 RTC；Sensor 保持原有硬件 RTC 工厂测试逻辑。

电池硬件逻辑统一放在 `esp32_s3_touch_rlcd_4_2.c`，由 `main.cpp` 调用 `bsp_battery_init()`。GPIO4 对应 ADC1 通道 3，采用 12-bit、12 dB 衰减和芯片校准；R21=200 kΩ、R23=100 kΩ，因此电池电压为校准后 ADC 毫伏值 ×3。每 500 ms 由 ESP 定时器执行一次短采样，累计 10 次求平均，再做 1/4 指数平滑，不新建任务或占用 LVGL 进行 ADC 读取。

顶部约上电 5 秒后显示首个百分比，随后每 5 秒更新电量缓存；百分比沿用小智 RLCD-4.2 的电压估算公式并限制在 0–100%，不是精密电量计。初始化失败、首次结果前或有效结果超过 15 秒未更新时显示 `--%`；ADC 错误、饱和及不合理电压不会被当成真实 0%/100%。串口首次成功输出 `Battery: ... mV, estimated ...%`，可与万用表核对。

充电时电压抬高会影响估算，且无法仅凭此电压可靠识别电池是否插入。软件不判断充电状态、不显示充电动画，充电情况看板上指示灯。已有 `rlcd_desktop_set_battery(percent, charging)` 接口保留，桌面固定传入 `false`。所有 UI setter 需在持有 LVGL 锁时调用；传入无效值恢复占位。

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。
