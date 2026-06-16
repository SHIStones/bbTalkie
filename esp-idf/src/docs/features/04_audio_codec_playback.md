# 音频编解码与播放

## 功能说明

本工程使用 16 kHz、16 bit、单声道 PCM 作为内部语音格式。无线传输前用 ADPCM 压缩，远端收到后解码为 PCM，再写入 I2S 播放。

播放前会做一个简单 AGC，避免远端声音过小或过大。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `bb_audio.c` | ADPCM 编解码、播放缓冲、AGC、提示音、播放任务。 |
| `bb_audio.h` | 对外音频接口。 |
| `agc.h` | AGC 参数和限制。 |
| `app_config.h` | 采样率、位深、帧长、缓冲大小。 |
| `bsp_board.c` | `esp_audio_play()` 最终调用的 I2S TX 实现。 |

## 调用关系

发送端编码：

```text
detect_Task()
  -> encode_and_send()
  -> encode_adpcm()
  -> send_data_esp_now()
```

接收端播放：

```text
decode_Task()
  -> get_esp_now_data()
  -> decode_adpcm()
  -> xStreamBufferSend(play_stream_buf)

i2s_writer_task()
  -> xStreamBufferReceive(play_stream_buf)
  -> apply_agc()
  -> esp_audio_play()
  -> bsp_audio_play()
  -> i2s_channel_write(tx_handle)
```

提示音：

```text
boot_sound() / byebye_sound()
  -> esp_audio_play()
```

## 主要函数如何使用

### `encode_adpcm()`

用途：把 PCM 压缩成 ADPCM。

调用方式：

```c
uint8_t adpcm_output[ENCODED_BUF_SIZE];
size_t adpcm_len = 0;

encode_adpcm(pcm_data,
             pcm_len_bytes,
             adpcm_output,
             &adpcm_len);
```

输入要求：

- `pcm_data` 是 `int16_t *`。
- `pcm_len_bytes` 是字节数，不是样本数。
- 当前配置按 `SAMPLE_RATE = 16000`、`BIT_DEPTH = 16`、单声道编码。

### `decode_adpcm()`

用途：把 ADPCM 解码成 PCM。

调用方式：

```c
uint8_t pcm_output[ENCODED_BUF_SIZE];
size_t pcm_len = 0;

decode_adpcm(adpcm_data,
             adpcm_len,
             pcm_output,
             &pcm_len);
```

注意：当前实现最后固定设置：

```c
*pcm_len = ADPCM_FRAME_SIZE * 2;
```

如果后续支持可变帧长，需要改这里。

### `init_audio_stream_buffer()`

用途：创建播放缓冲 `play_stream_buf`。

调用方式：

```c
init_audio_stream_buffer();
```

必须在 `decode_Task()` 和 `i2s_writer_task()` 启动前调用。

### `decode_Task()`

用途：从无线接收队列取 ADPCM，解码后写入播放流缓冲。

调用方式：

```c
xTaskCreatePinnedToCore(decode_Task, "decode", 4 * 1024, NULL, 5, NULL, 0);
```

状态行为：

- 收到音频后设置 `is_receiving = true`。
- 超过 128 ms 没收到音频后设置 `is_receiving = false`。
- 停止接收时播放短静音，帮助 I2S 输出收尾。

### `i2s_writer_task()`

用途：播放缓冲中的 PCM。

调用方式：

```c
xTaskCreatePinnedToCore(i2s_writer_task, "i2sWriter", 4 * 1024, NULL, 5, NULL, 0);
```

内部逻辑：

```c
xStreamBufferReceive(play_stream_buf, i2s_buf, PLAY_CHUNK_SIZE, portMAX_DELAY);
apply_agc((int16_t *)i2s_buf, received / 2, &agc_custom);
esp_audio_play((const int16_t *)i2s_buf, received / 2, portMAX_DELAY);
```

如果 `isMute == true`，收到的数据不会播放。

### `apply_agc()`

用途：对 PCM 原地自动增益控制。

调用方式：

```c
apply_agc((int16_t *)i2s_buf, received / 2, &agc_custom);
```

参数在 `agc_custom` 中配置，定义在 `app_state.c`。

## 修改入口

- 调整编码帧长：`app_config.h` 的 `ADPCM_FRAME_SIZE`。
- 调整播放缓冲：`PLAY_RING_BUFFER_SIZE`、`PLAY_CHUNK_SIZE`。
- 调整 AGC：`agc.h` 和 `app_state.c` 的 `agc_custom`。
- 改编码格式：替换 `encode_adpcm()` / `decode_adpcm()`。
- 改播放声道：`bsp_board.c` 的 `bsp_audio_play()`。

