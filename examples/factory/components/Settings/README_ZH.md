# Settings — 400 × 300 RLCD

[English](README.md) | [简体中文](README_ZH.md)

参照 C5-2.8 的 Settings 生命周期和网络功能移植，使用 LVGL 原生黑白控件。
在 `main/main.cpp` 注册为第一个 APP；桌面图标和页面图标为原生 I1 数组。

## 页面

- Settings：首页标题下方是 Volume 滑杆，下方保留 Wi-Fi、Network setup、SD Card、About 四个入口。
  音量范围 0～100，每档 5，未保存过时默认 55。拖动实时更新请求值，松手后后台保存到 NVS；
  页面切换、暂停或关闭时也提交尚未保存的拖动值。切歌、退出重进及重启均保留音量。
  音量 0 会关闭功放并静音，播放进度继续；恢复音量不会自动恢复已暂停的播放。
  保存失败显示 `Save failed`，本次音量仍生效，可重新调节重试。
  GPIO18 控制的微笑/闭嘴表情已移到桌面顶部中央，由 `../rlcd_desktop` 管理；设置页不再创建表情或按键轮询。
- Wi-Fi：仅扫描 2.4 GHz，按本次扫描的实际数量动态保存全部结果，每页 3 条，包含名称、RSSI、信道、加密方式。
  不能点击网络连接，也不修改保存的信息。非 ASCII 名称按 C5 用 `?` 占位。
  `<hidden>` 仅用于扫描记录中的空 SSID，不代表中文；5 GHz 不在 ESP32-S3 扫描范围内。
  内存不足或驱动读取失败会报告扫描失败，不会静默截成 8 条。UI 只复制当前页，避免大栈占用。
- Network setup：点击后直接进入二维码配网页，移除重复的 Hotspot 入口和连接方式选择页。返回直接回 Settings 首页。
  QR 内容沿用 C5 的 WIFI 格式，作用是加入设备 SoftAP；随后在手机网页输入路由器信息。
  AP 名称为 `RLCD4.2-xxxxxx`，密码 `12345678`，网址 `http://192.168.4.1`。
  手机配网页面位于 `components/network_service/root.html`，统一使用英文，包含输入校验、连接状态和错误提示；成功后提示关闭网页，不再提及天气 APP。网页嵌入固件，修改后需重新编译烧录。
  成功获取 IP 后保存到 NVS，约 5 秒后关闭热点；退出或暂停页面也关闭热点。
  已提交的路由器连接可以在后台完成。启动优先读取已保存信息；未配网时仍使用工厂测试热点。
- SD Card：每次上电启动后自动在后台调用 BSP 挂载和容量查询，进入页面即可查看 FAT 卷总量及剩余空间，无需先点击检测；尚未完成时显示 Checking...。
  保留 Check card 按钮，插卡、更换卡或需要刷新剩余空间时可手动重新检测。
  不是格式化或弹出工具，也不提供自动热插拔检测；状态是上次检查的结果。
  不扫描 `/photo`，不解码图片，相册仍在打开后扫描/解析并按原策略预取。
- About：固件版本、IDF、STA MAC、Flash/PSRAM 容量和屏幕信息。
  固件版本来自根 CMakeLists.txt 的 PROJECT_VER，当前为 V1.0。
  SD 的 GiB/MiB 是二进制容量单位：1 GiB=1024^3 字节，1 MiB=1024^2 字节。

## 分层

- `Settings.cpp/.hpp`：Brookesia APP 生命周期、返回和暂停/恢复。
- `ui/settings_ui.c`：布局、页面切换、250 ms 快照刷新，仅变化时更新控件。
- `assets/settings_icons.c`：黑白 C 数组，桌面 64 × 64，页内图标 32 × 32。
- `service/settings_service.c`：后台 SD 检查、音量保存、设备信息快照，以及网络服务接口转接。
  SD 检查和音量保存分别排队，保存音量不会挂载 SD；积压的音量保存采用最新松手值。
- BSP `bsp_audio_volume.c`：统一音量缓存及 NVS `rlcd_audio/volume`；在 main 初始化 NVS 后加载。
  GUI 只更新内存，播放任务在 PCM 输出或恢复播放时应用音量；codec 访问与暂停/关闭互斥。
- `../network_service/wifi_manager.c`：唯一 Wi-Fi 工作任务，串行管理扫描、连接、热点、配网和 NVS。
  扫描不阻塞 GUI；已有连接尽量保留，尚未连接时暂缓自动连接以完成扫描。
  配网使用会话编号，避免快速退出/重进时使用旧结果或旧关闭定时。
- BSP `esp32_s3_touch_rlcd_4_2b.c/.h`：统一维护 `bsp_sdcard_mount()`、
  `bsp_sdcard_unmount()`、`bsp_sdcard_get_info()`，加锁避免重复挂载。
  Photos 在错误恢复时也通过 BSP 管理共享卡句柄。
  `bsp_user_button_init()` 在 main 启动时调用，`bsp_user_button_get_press_count()` 提供消抖后的计数。
  按键定时器只更新计数，不直接访问 LVGL，也不被 SD 容量查询阻塞。

## 编译与验证

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。
