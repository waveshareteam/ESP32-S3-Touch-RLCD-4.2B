# RLCD 小智语音 APP

[English](README.md) | [简体中文](README_ZH.md)

移植自 the reference Xiaozhi application，保留激活、WebSocket 对话、Opus 编解码、WakeNet、消回声和语音打断流程。音频硬件配置同时核对了 the upstream Waveshare RLCD board definition 与 `BoxAudioCodec`。显示改为 400×300 的 LVGL I1 黑白界面，在 Brookesia 桌面显示为 AIChats。

## 使用

1. 用现有 Settings 配网功能连接可访问小智服务的 2.4 GHz 网络，再打开桌面的 AIChats。
2. 未绑定的设备会显示激活码，在小智控制面板添加设备并输入该码。身份由本机 MAC 和 NVS 中生成的 UUID 决定，不使用参考设备的身份。
3. 显示 Standby 后说“你好小智”或点击 Talk。说完由服务判断结束并回复；Stop 取消当前对话。小智讲话时可再次说唤醒词打断。
4. 右侧显示最近一条识别结果或回复，长文本可以上下滚动。右上角返回按钮和原有底部导航手势可返回桌面。
5. 音量使用 Settings 的 Audio 音量值，默认 55，0 为静音。小智离开页面后释放音频；音乐离开页面也会停止并释放音频，返回音乐保留曲目选择，点击 Play 从该曲开头播放。

支持通过语音说“把音量调到 50%”“声音大一点”“声音小一点”或“现在音量多少”。连接小智服务时注册 `self.get_device_status` 和 `self.audio_speaker.set_volume` 两个 MCP 工具；相对调节先查询当前值，再设置 0–100 的目标音量。执行日志为 `Speaker volume set to ...% by MCP`，实际语音识别和工具调用由小智服务完成。

语音调节更新与 Settings、Music 共用的运行时音量，播放任务在下一个 PCM 块或恢复播放时应用；0 为静音，麦克风采集继续工作。语音调用不写入 Flash，重启后读取 Settings 中保存的音量；需要记忆的值可在 Settings 的 Audio 页面设置并保存。

离开小智页面后不在后台监听唤醒词。网络断开后显示 Wi-Fi required，恢复网络后自动重试。APP 复用 `network_service`，不创建第二套 Wi-Fi 配网服务。

## 音频与 BSP

硬件操作统一放在 `components/esp32_s3_touch_rlcd_4_2/esp32_s3_touch_rlcd_4_2.c`，公开接口在同组件 `include/bsp/esp32_s3_touch_rlcd_4_2.h`。

| 信号 | RLCD GPIO |
| --- | --- |
| I2C SDA / SCL | 13 / 14 |
| I2S MCLK / BCLK / WS | 16 / 9 / 45 |
| I2S DIN / DOUT | 10 / 8 |
| 功放使能 | 46 |

ES8311 输出使用 24 kHz、16 bit、标准 I2S 双声道；每个单声道语音采样复制到左右槽。ES7210 输入使用同一时钟的四槽 TDM，保留 2.06 的通道顺序：

| TDM 槽 | 物理输入 | 用途 |
| --- | --- | --- |
| 0 | MIC1 | 第一麦克风 |
| 1 | MIC3 | 喇叭回声参考 |
| 2 | MIC2 | 第二麦克风 |
| 3 | MIC4 | 不送入算法 |

输入从 24 kHz 分通道重采样为 16 kHz，重排为 `MIC1, MIC2, Reference`，送入格式为 `MMR`、开启 AEC 的 AFE。随后编码为 16 kHz、单声道、60 ms 的 Opus 包上行。下行 Opus 按服务声明的采样率解码并转换为 24 kHz 播放。

AFE 是音频前端，AEC 是其中的回声消除模块。原理图中的 `OUTP/OUTN` 通过参考信号电路接到 ES7210 的 `MIC3P/MIC3N`，MIC3 用于采集播放参考，并不替代 MIC1/MIC2 的人声输入。日志明确打印硬件 TDM 槽顺序与送入 AFE 的 `MIC1/MIC2/MIC3(REF)` 顺序。

`bsp_audio_session_*` 管理 Music / Xiaozhi 对共享 I2S 和编解码器的独占使用；`bsp_audio_voice_*` 提供有超时的读写与关闭接口。I2S 保持 TX STD / RX TDM，切换 APP 时由 codec open 配置采样率和有效槽，不反复销毁总线。Music 的输出仍为 24 kHz 单声道。

## 文件入口

- `XiaozhiApp.cpp/.hpp`：APP 生命周期、状态机、音频任务与对话控制。
- `XiaozhiAudioProcessor.cpp`：双麦克风 AFE、AEC、VAD、WakeNet。
- `XiaozhiClient.cpp`：WebSocket / MQTT 协议适配；默认 WebSocket，与 2.06 一致。
- `XiaozhiActivationClient.cpp`：设备身份、激活和连接参数获取。
- `XiaozhiUi.cpp`：黑白界面、中文对话、激活页、触摸操作。
- `assets/img_app_xiaozhi.c`：用户提供的机器人图片转换成的 64×64 I1 黑白桌面图标；原图为 `assets/xiaozhi_icon_source.png`，预览为 `assets/xiaozhi_icon_rlcd.png`。
- `XiaozhiDeveloperTools.cpp`：注册设备音量查询和扬声器音量设置 MCP 工具，与 BSP 共享音量接口对接。

中文字体直接链接进固件，显示不依赖 SD 卡或 SPIFFS。代码保留了可选激活提示音播放器，但参考组件没有附带 OGG 音频，本工程也未挂载提示音文件系统，因此激活码默认通过屏幕展示。没有接入参考工程可选的 SD 聊天历史模块。

## 编译与首次烧录

在工程根目录的 ESP-IDF 5.5.4 环境运行：

```powershell
idf.py build
idf.py -p COMx flash monitor
```

将 `COMx` 替换为实际串口。第一次使用这个版本必须完整 flash，包含分区表和 `build/srmodels/srmodels.bin`，不要只执行 app-flash。烧录参数以 `build/flash_args` 为准：

| 文件 | 地址 |
| --- | --- |
| bootloader/bootloader.bin | 0x0 |
| partition_table/partition-table.bin | 0x8000 |
| esp32_s3_touch_rlcd_4_2.bin | 0x10000 |
| srmodels/srmodels.bin | 0xF10000 |

原 NVS、8 MiB APP 和 7 MiB storage 的位置、大小均保留，在 16 MiB Flash 的剩余尾部加入 960 KiB 的 model 分区。当前只启用“你好小智”模型。新增中文字体及语音常量较大，配置保留指令从 PSRAM 执行、常量留在 Flash，给语音处理留出更多 PSRAM 工作空间。

## 内存分配与安装失败处理

控制队列（15,936 字节）、PCM 队列（76,800 字节）和播放队列（15,160 字节）通过 `xQueueCreateWithCaps` 显式分配到 PSRAM，并用 `vQueueDeleteWithCaps` 成对释放。控制与播放队列此次从内部 RAM 迁出，减少约 31 KB 的内部 RAM 占用；PCM 队列原本就在 PSRAM。这些队列仅由任务访问，必要的同步对象及 DMA 缓冲仍使用内部 RAM。

普通堆分配策略对齐 2.06：`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=0`、`CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=4096`。PSRAM 与内部 RAM 都是所有 APP 共用的有限资源；分配器隔离各个分配块，共享音频硬件仍由 BSP 会话接口管理。

AFE 处理任务的 8 KiB 栈与采集任务的 6 KiB 栈通过 `xTaskCreateWithCaps` / `xTaskCreatePinnedToCoreWithCaps` 显式分配到 PSRAM，任务优先级、核绑定和栈大小保留。这样可减少 14 KiB 的片内栈需求；任务控制块、同步对象和 DMA 缓冲仍使用内部 RAM。`sdkconfig.defaults` 使用 ESP-IDF 5.5 的 `CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM=y`，该选项允许外部栈，但不会自动改变普通 `xTaskCreate` 的分配位置。

这两个任务退出后通知所属控制任务，由控制任务调用 `vTaskDeleteWithCaps` 回收任务栈及控制块，再释放 AFE、重采样器及音频会话。任务自身不另建清理任务，降低内存紧张时退出失败的风险。采集缓冲申请失败、后续音频任务创建失败、正常退出和再次进入均沿用相同回收路径。

AFE 初始化前、处理任务创建前及相关失败分支会打印 `AFE heap`，包括内部 RAM / PSRAM 的剩余量和最大连续块，数值单位为字节。成功时可看到 `Ready: input=MMR ... stack=8192 bytes in PSRAM, AEC reference=MIC3`，随后出现 `Audio tasks ready: input stack=6144 bytes in PSRAM`。这些日志用于区分模型初始化失败、任务内存不足和后续音频链路问题；实际回声消除和多次进出应用仍需上板验证。

初始化会输出 `Service info heap`，包含内部 RAM / PSRAM 的剩余量和最大连续块；同步对象申请失败时会标明失败对象。成功时可看到 `Xiaozhi queues in PSRAM: control=15936, PCM=76800, playback=15160 bytes`。原日志不能确定具体是哪个对象申请失败，需以新固件上板日志确认实际内存状态。

小智安装失败时清理自身资源并保留桌面及其他 APP。Brookesia 的失败回滚同时修正为清理未完成的初始化、移除正确的注册项，支持失败后重试。

## 编译与验证

电脑端测试与辅助转换脚本已移出固件源码，原文件保存在工程上一层的 `backup_before_app_cleanup_20261015.zip`。固件通过 `idf.py build` 编译；实际外设、触摸和运行效果需上板验证。

小智上板需检查首次激活、唤醒、录音、扬声器播放、语音打断、断网恢复及 Music / Xiaozhi 切换。语音唤醒模型和完整烧录流程仍按上文执行。
