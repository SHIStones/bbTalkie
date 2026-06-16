# 对讲功能

## 功能说明

本工程的对讲不是按住说话的 PTT，而是声控半双工对讲：

- 本机 AFE/VAD 检测到说话后，自动把语音编码成 ADPCM。
- 通过 ESP-NOW 广播给附近设备。
- 远端收到后解码为 PCM，再通过 I2S 功放播放。
- 正在接收远端语音时，本机不发送语音，避免互相抢话。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `main.c` | 创建 AFE、启动采集/检测/解码/播放任务。 |
| `bb_audio.c` | VAD 检测、ADPCM 编解码、播放缓冲和 I2S 播放。 |
| `bb_radio.c` | ESP-NOW 发送、接收回调和接收队列。 |
| `app_state.c/.h` | `is_speaking`、`is_receiving`、队列和播放缓冲等共享状态。 |
| `app_config.h` | 采样率、包长、ADPCM 帧长等配置。 |

## 调用关系

发送链路：

```text
feed_Task()
  -> esp_get_feed_data()
  -> afe_handle->feed()

detect_Task()
  -> afe_handle->fetch()
  -> 判断 res->vad_state
  -> encode_and_send()
  -> encode_adpcm()
  -> send_data_esp_now()
  -> esp_now_send()
```

接收链路：

```text
esp_now_recv_cb()
  -> 判断不是 PING/CMD/MSG
  -> xQueueSend(s_recv_queue)

decode_Task()
  -> get_esp_now_data()
  -> decode_adpcm()
  -> xStreamBufferSend(play_stream_buf)

i2s_writer_task()
  -> xStreamBufferReceive(play_stream_buf)
  -> apply_agc()
  -> esp_audio_play()
  -> bsp_audio_play()
  -> i2s_channel_write()
```

## 主要函数如何使用

### `feed_Task(void *arg)`

用途：持续从 I2S 读双通道音频，并喂给 ESP-SR AFE。

调用方式：

```c
xTaskCreatePinnedToCore(feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
```

要求：

- `arg` 必须是 `esp_afe_sr_data_t *`。
- 必须先完成 `esp_board_init()` 和 `afe_handle->create_from_config()`。

### `detect_Task(void *arg)`

用途：从 AFE 获取处理后的音频，做 VAD 和 MultiNet 检测；检测到语音时发送音频。

调用方式：

```c
xTaskCreatePinnedToCore(detect_Task, "detect", 8 * 1024, (void *)afe_data, 5, NULL, 1);
```

关键判断：

```c
if ((res->vad_state != VAD_SILENCE) &&
    (is_receiving == false) &&
    (isMicOff == false))
```

这表示只有在非静音、未接收远端语音、麦克风未关闭时才发送。

### `encode_adpcm()`

用途：把 16 kHz/16 bit/mono PCM 压缩为 ADPCM。

调用方式：

```c
size_t adpcm_len = 0;
encode_adpcm(pcm_data, pcm_len_bytes, adpcm_output, &adpcm_len);
```

注意：

- 输入 PCM 以字节数传入。
- 输出缓冲建议使用 `ENCODED_BUF_SIZE`。
- 当前封装由 `encode_and_send()` 自动处理，业务代码通常不直接调用。

### `send_data_esp_now()`

用途：广播一段数据。长度超过 `ESP_NOW_PACKET_SIZE` 时自动切包。

调用方式：

```c
send_data_esp_now(adpcm_output, adpcm_len);
```

注意：

- 当前广播给 `FF:FF:FF:FF:FF:FF`。
- 每个分片之间有 16 ms 延时。

### `decode_Task(void *arg)`

用途：从 ESP-NOW 接收队列取 ADPCM 音频，解码后写入播放缓冲。

调用方式：

```c
xTaskCreatePinnedToCore(decode_Task, "decode", 4 * 1024, NULL, 5, NULL, 0);
```

### `i2s_writer_task(void *arg)`

用途：从 `play_stream_buf` 取 PCM，做 AGC，然后播放。

调用方式：

```c
xTaskCreatePinnedToCore(i2s_writer_task, "i2sWriter", 4 * 1024, NULL, 5, NULL, 0);
```

## 增加或修改功能

### 改成按键 PTT

当前由 VAD 自动发送。若改为按住说话，可新增一个 `isPttPressed` 状态，然后把 `detect_Task()` 的发送条件改为：

```c
if ((res->vad_state != VAD_SILENCE) &&
    (is_receiving == false) &&
    (isMicOff == false) &&
    (isPttPressed == true))
```

按键按下/释放事件可在 `bb_button.c` 中注册。

### 调整音质和延迟

相关宏在 `app_config.h`：

- `SAMPLE_RATE`：当前 16000。
- `ADPCM_FRAME_SIZE`：当前 505，影响编码帧大小和延迟。
- `ESP_NOW_PACKET_SIZE`：当前 512，影响无线分片。
- `PLAY_RING_BUFFER_SIZE` / `PLAY_CHUNK_SIZE`：影响播放缓冲和卡顿。

减小帧长可以降低延迟，但包更多，丢包和调度压力会变大。

### 支持私聊

当前是广播。若要指定设备：

1. 在 `mac_track_list` 中选择目标 MAC。
2. 用 `esp_now_add_peer()` 添加目标 peer。
3. 把 `send_data_esp_now()` 内部的 `broadcast_mac` 换成目标 MAC。

