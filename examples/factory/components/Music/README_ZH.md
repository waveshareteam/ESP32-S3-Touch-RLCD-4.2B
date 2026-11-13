# RLCD Music

[English](README.md) | [简体中文](README_ZH.md)

400×300 黑白音乐播放器，保留山月封面布局。桌面音符图标由用户提供的 PNG 转成 LVGL I1 C 数组。顶栏使用 `rlcd_ui` 的统一黑底白字样式，左侧图标和 Music 名称，右上角白色返回箭头退出并关闭播放器，曲目序号移到顶栏下方。默认暂停，提供上一首、播放/暂停、下一首。进度条只显示，不支持拖动。

## 使用

在 FAT32 SD 卡根目录创建 `music` 文件夹，放入 `.mp3` / `.MP3` 文件，例如：

```
/music/01_Slow_Morning.mp3
/music/02_Ocean_Breeze.mp3
```

进入 Music 时才挂载/复用 SD 卡并扫描该文件夹，不递归子目录，不在启动时解码。文件按名称排序；建议名称前加 `01_`、`02_` 固定顺序。点击中间 ▶ 播放，点击 Ⅱ 暂停并保留位置。手动上一首/下一首循环选择，保留当前播放/暂停状态；当前曲目播放结束后停在末尾，再按播放从头开始。中间向上滑返回并关闭播放器。

曲目就绪后，上方固定显示 `Music Player`，下方固定显示 `Chill Vibes`，播放和暂停时均不显示文件名或 ID3 曲名/歌手，避免中文字形缺失或标签编码异常影响界面。曲目序号、播放时间和进度仍按实际曲目更新。加载、空文件夹、SD 不可用、损坏 MP3 或音频初始化失败时仍显示对应提示；错误状态下中间播放键重新扫描，恢复后再按播放。

## 结构

- `Music.cpp/.hpp`：与 Games 相同的 Brookesia APP 封装，在 `main/main.cpp` 注册。
- `ui/music_ui.c`：纯黑白 LVGL 界面，保留上滑退出。每 200 ms 检查状态，时间/进度按整秒变化且只更新变化的对象，没有滚动字幕、进度动画或封面动画。
- `service/music_service.c`：后台工作任务管理扫描、切歌、播放器和文件生命周期，UI 不直接做 SD/I2S 操作。过期请求不会回写已关闭/已切换的页面。
- `service/music_service.c/.h` 同时集中管理原 model 的数据结构、文件排序、MP3 MPEG Layer III 时长解析及基础 ID3 标题/歌手读取。Xing/Info、VBRI 帧数可快速得到时长；无帧数信息时扫描全部帧，覆盖 CBR/VBR。自由码率/损坏帧不作为有效 MP3 接受。
- `assets/`：用户图标源图、确认的山月封面、I1 C 数组，直接参与固件编译。

播放器使用参考 the reference factory application 的 `esp_audio_simple_player 0.9.6` / ESP-GMF 解码架构；改用本地 FILE 回调提供 MP3 流，避免文件名空格、`#` 被当作 URI。ES8311 驱动保持原项目 `esp_codec_dev 1.5.10`。输出固定为 24000 Hz、单声道、16 bit，转换配置在 `sdkconfig` 和 `sdkconfig.defaults`。音量由 Settings 首页的 Volume 调节，首次默认 55，范围 0～100，每档 5；BSP 统一管理，切歌/重开不重置，松手后后台保存以便重启恢复。0 档同时关闭功放和静音，时间仍按正常播放推进。

硬件全部通过 `components/esp32_s3_touch_rlcd_4_2b` 的 BSP 接口管理：

- `bsp_audio_playback_open/write/drain/mute/close`：I2S、ES8311、功放；PA 在暂停/退出时关闭。
- 现有音频引脚：MCLK=16、BCLK=9、LRCLK=45、DOUT=8、PA=46；I2C SDA=13/SCL=14。不初始化麦克风。
- `bsp_sdcard_acquire/release`：长时间读取期间保持 SD 占用，BSP 拒绝其他使用者卸载；释放不卸载共享卡，不改变相册启动行为。

已播放时间由成功提交给 I2S 的 PCM 字节数计算，精度受短 DMA 队列延迟影响，不依赖 LVGL 定时器累加或 SD 文件读取偏移。暂停/EOF 会等待 DMA 尾部数据播完再静音。未知总时长显示 `--:--`。

## 编译与验证

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。
