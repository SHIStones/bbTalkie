# bbTalkie 功能文档索引

本文档目录按功能说明 bbTalkie 固件的实现方式、函数用法和调用关系。适合后续增加、替换或调试某个功能时查阅。

## 文档列表

| 文档 | 说明 |
| --- | --- |
| `01_walkie_talkie.md` | 对讲功能：自动检测说话、ADPCM 编码、ESP-NOW 广播、远端解码播放。 |
| `02_echo_cancellation.md` | 回声消除：I2S loopback 原理、`RM` 双通道输入、AFE 处理链。 |
| `03_esp_now_radio.md` | ESP-NOW 无线协议：初始化、广播、接收回调、PING/CMD/MSG/音频包分类。 |
| `04_audio_codec_playback.md` | 音频编解码和播放：ADPCM、播放缓冲、AGC、I2S 输出。 |
| `05_voice_command.md` | 语音命令：AFE fetch、VAD、MultiNet、命令动画和消息广播。 |
| `06_display_animation.md` | OLED 显示和动画：状态栏、主界面、命令动画、消息气泡、充电图标。 |
| `07_button_power.md` | 按键、电源和充电：单击/双击/长按、关机、充电模式、深睡眠。 |
| `08_led_status.md` | WS2812 状态灯：初始化、颜色、呼吸灯、说话/接收/空闲状态。 |
| `09_configuration_build.md` | 配置和编译：关键宏、组件依赖、VSCode/ESP-IDF 构建注意事项。 |

## 主启动关系

入口在 `esp-idf/src/main/main.c` 的 `app_main()`。它主要完成：

1. 初始化 NVS、GPIO、按键、ESP-NOW。
2. 初始化板级 I2S 音频和 ESP-SR AFE。
3. 创建播放缓冲区。
4. 创建 OLED、提示音、采集、检测、接收解码、播放等 FreeRTOS 任务。

主任务启动关系：

```text
app_main()
  -> ws2812_init()
  -> init_button()
  -> init_esp_now()
  -> esp_board_init()
  -> afe_config_init()
  -> afe_handle->create_from_config()
  -> init_audio_stream_buffer()
  -> xTaskCreate(oled_task)
  -> xTaskCreate(boot_sound)
  -> xTaskCreate(feed_Task)
  -> xTaskCreate(detect_Task)
  -> xTaskCreate(decode_Task)
  -> xTaskCreate(i2s_writer_task)
  -> xTaskCreate(ping_task)
  -> xTaskCreate(led_control_task)
```

如果检测到充电/唤醒条件，`app_main()` 会提前进入 `charging_Task()`，不再启动完整对讲链路。

## 共享状态

跨任务状态集中在 `app_state.c` / `include/app_state.h`：

- `is_receiving`：正在接收远端语音。
- `is_speaking`：本机正在说话。
- `isMicOff`：麦克风关闭。
- `isMute`：本地播放静音。
- `play_stream_buf`：播放 PCM 数据流缓冲。
- `s_recv_queue`：ESP-NOW 接收音频包队列。
- `afe_handle` / `models`：ESP-SR AFE 和模型句柄。

新增功能时优先复用这些状态；如果新增跨任务状态，应先评估是否需要 `volatile`、队列、互斥锁或事件组。
