# bbTalkie 项目功能与模块说明

## 项目用途

bbTalkie 是一个基于 ESP32-S3 的迷你免提对讲机固件。设备通过 I2S 麦克风采集语音，使用 ESP-SR AFE/VAD/MultiNet 做语音活动检测和中文命令识别，并通过 ESP-NOW 在多台设备之间广播 ADPCM 压缩后的语音帧。OLED 用于显示开机动画、说话/接收/空闲状态、电池状态、在线节点数和命令动画，WS2812 用不同颜色提示当前工作状态。

典型交互如下：

- 说话时：本机检测到 VAD 语音，编码为 ADPCM 后通过 ESP-NOW 广播，同时 OLED/LED 显示说话状态。
- 收到语音时：无线接收 ADPCM 帧，解码为 PCM 后播放，同时 OLED/LED 显示接收状态。
- 识别到命令时：本机播放对应 OLED 命令动画，并广播 `CMD:<id>`，远端设备同步显示同一命令动画。
- 识别超时文本时：本机显示文本气泡，并广播 `MSG:<text>`，远端设备同步显示消息气泡。
- 单击按键：退出命令动画或切换麦克风关闭状态。
- 双击按键：切换静音播放。
- 长按按键：播放关机音效和关机动画，然后进入深睡眠。
- 充电唤醒：显示大电池充电界面，充满为绿色灯，充电中为橙色灯。

## 模块划分

源码位于 `esp-idf/src/main`，入口仍是 ESP-IDF 组件 `main`。

| 文件 | 作用 |
| --- | --- |
| `main.c` | 应用入口，只负责 NVS、GPIO、ESP-NOW、音频板卡、ESP-SR 模型初始化和任务启动。 |
| `app_state.c` / `include/app_state.h` | 跨任务共享状态：语音/接收/静音/关机标志、OLED 句柄、ESP-NOW 队列、字体、AFE 句柄等。 |
| `include/app_config.h` | 硬件引脚、采样率、ADPCM 帧长、ESP-NOW 包长、按键和唤醒 GPIO 配置。 |
| `bb_radio.c` / `include/bb_radio.h` | ESP-NOW 初始化、广播发送、接收队列、PING 在线节点维护、`CMD:`/`MSG:` 消息解析。 |
| `bb_audio.c` / `include/bb_audio.h` | I2S 麦克风喂 AFE、VAD/MultiNet 检测、ADPCM 编码发送、ADPCM 解码播放、AGC、开关机音效。 |
| `bb_display.c` / `include/bb_display.h` | SSD1327 OLED 初始化、状态栏、电池图标、主界面状态机、消息气泡、关机动画。 |
| `bb_animation.c` / `include/animation.h` / `include/command_map.h` | 主界面动画对象和中文命令 ID 到自定义动画的映射。 |
| `bb_led.c` / `include/led.h` | WS2812 初始化、颜色设置、空闲/说话/接收呼吸灯状态。 |
| `bb_button.c` / `include/bb_button.h` | 按键初始化和单击、双击、长按回调。 |
| `bb_power.c` / `include/bb_power.h` | 充电唤醒后的大电池界面、充电 LED、未充电时进入深睡眠。 |

## 修改入口

- 修改硬件引脚、采样率、包长：改 `include/app_config.h`。
- 修改语音发送/接收协议：改 `bb_radio.c` 和 `bb_audio.c`。
- 修改中文命令对应动画：改 `bb_animation.c` 的 `animation_map`。
- 修改 OLED 页面和状态动画：改 `bb_display.c`。
- 修改按键行为：改 `bb_button.c`。
- 修改 LED 颜色：改 `include/led.h` 的颜色宏或 `bb_led.c` 状态机。

## 功能文档

更细的功能说明、函数用法和调用关系已拆分到 `docs/features/`：

- `docs/features/01_walkie_talkie.md`：对讲功能。
- `docs/features/02_echo_cancellation.md`：回声消除。
- `docs/features/03_esp_now_radio.md`：ESP-NOW 无线通信。
- `docs/features/04_audio_codec_playback.md`：音频编解码和播放。
- `docs/features/05_voice_command.md`：语音命令识别。
- `docs/features/06_display_animation.md`：OLED 显示和动画。
- `docs/features/07_button_power.md`：按键、电源与充电。
- `docs/features/08_led_status.md`：WS2812 状态灯。
- `docs/features/09_configuration_build.md`：配置与编译。

## VSCode 编译使用

1. 用 VSCode 打开仓库根目录 `E:\bbTalkie`，或打开 `bbTalkie.code-workspace`。
2. 确认 ESP-IDF 插件选择的框架为 `D:\Espressif\frameworks\esp-idf-v5.5.1`。
3. 目标芯片选择 `esp32s3`。
4. 直接执行 ESP-IDF 插件的 `Build`。
5. 编译成功后主要产物为 `build/wake_word_detection.bin`。

如果命令行编译，需要先加载 ESP-IDF 环境。当前机器的 `export.ps1` 可能会误找 `idf5.5_py3.12_env`，而 VSCode 日志使用的是 `D:\Espressif\tools\python_env\idf5.5_py3.11_env`。在 VSCode 插件里编译通常不需要手工处理；若手工命令行遇到 Python 依赖检查，请优先修复 ESP-IDF Python 环境，而不是改项目源码。

## 本次重构验证

已使用 ESP-IDF v5.5.1、目标 `esp32s3` 重新配置并编译通过：

- `build/bootloader/bootloader.bin`
- `build/wake_word_detection.bin`

当前应用镜像大小约 `0x4c18d0` 字节，最小 app 分区 `0x7d0000` 字节，剩余约 39%。
