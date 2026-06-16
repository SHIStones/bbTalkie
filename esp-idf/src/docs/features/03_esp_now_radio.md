# ESP-NOW 无线通信

## 功能说明

ESP-NOW 是 Espressif 的无连接 Wi-Fi 小包通信机制。它不需要连接路由器，也不使用 TCP/IP socket。本工程用它广播：

- ADPCM 语音帧。
- `PING` 在线心跳。
- `CMD:<id>` 语音命令动画。
- `MSG:<text>` 文本消息气泡。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `bb_radio.c` | ESP-NOW 初始化、发送、接收、消息分类、在线节点维护。 |
| `bb_radio.h` | 对外接口。 |
| `app_state.c/.h` | 广播 MAC、接收队列、在线 MAC 表。 |
| `app_config.h` | 包长、PING、在线超时等宏。 |

## 调用关系

初始化：

```text
app_main()
  -> init_esp_now()
    -> esp_netif_init()
    -> esp_event_loop_create_default()
    -> esp_wifi_init()
    -> esp_wifi_set_mode(WIFI_MODE_STA)
    -> esp_wifi_start()
    -> esp_now_init()
    -> esp_now_register_send_cb()
    -> esp_now_register_recv_cb()
    -> esp_now_add_peer(broadcast_mac)
    -> xQueueCreate(s_recv_queue)
```

接收：

```text
ESP-NOW driver
  -> esp_now_recv_cb()
    -> PING: update_mac_track_list()
    -> CMD: handle_command_message()
    -> MSG: handle_text_message()
    -> audio: xQueueSend(s_recv_queue)
```

发送：

```text
业务代码
  -> send_data_esp_now()
  -> esp_now_send()
```

## 主要函数如何使用

### `init_esp_now()`

用途：初始化 Wi-Fi STA、ESP-NOW、广播 peer 和接收队列。

调用方式：

```c
if (init_esp_now() == false)
{
    // 初始化失败，查看日志
}
```

要求：

- 一般在 `app_main()` 早期调用。
- 需要在发送和接收前完成。

### `send_data_esp_now(const uint8_t *data, size_t len)`

用途：广播发送任意数据。超过 `ESP_NOW_PACKET_SIZE` 会自动分片。

调用方式：

```c
send_data_esp_now((const uint8_t *)"PING", 4);
send_data_esp_now(adpcm_output, adpcm_len);
```

注意：

- 当前 peer 是广播地址 `FF:FF:FF:FF:FF:FF`。
- 当前未加密。
- 分片之间延时 16 ms，避免连续发送压垮无线队列。

### `get_esp_now_data(esp_now_recv_data_t *recv_data)`

用途：非阻塞读取一帧音频数据。

调用方式：

```c
esp_now_recv_data_t recv_data;
if (get_esp_now_data(&recv_data) == true)
{
    decode_adpcm(recv_data.data, recv_data.data_len, pcm_buffer, &pcm_len);
}
```

注意：

- 只用于音频数据。
- `PING`、`CMD:`、`MSG:` 在接收回调里已经被处理，不会进入音频队列。

### `ping_task(void *arg)`

用途：每秒广播一次 `PING`，用于附近设备统计在线数量。

调用方式：

```c
xTaskCreate(ping_task, "ping", 2 * 1024, NULL, 5, NULL);
```

当前工程在 `app_main()` 中已经启动：

```c
xTaskCreate(ping_task, "ping", 3 * 1024, NULL, 5, NULL);
```

如果不需要在线设备数量，可以不创建该任务。

## 数据类型约定

| 数据内容 | 处理方式 |
| --- | --- |
| `PING` | 更新 `mac_track_list` 和 `macCount`。 |
| `CMD:<id>` | 调用 `bb_display_show_command_animation(id)`。 |
| `MSG:<text>` | 调用 `bb_display_show_message_bubble(text)`。 |
| 其它二进制数据 | 当成 ADPCM 音频帧放入接收队列。 |

## 增加或修改功能

### 增加新消息类型

在 `esp_now_recv_cb()` 中增加前缀判断：

```c
else if ((data_len >= 5) && (memcmp(data, "XXX:", 4) == 0))
{
    handle_xxx_message(data, data_len);
}
```

发送端使用：

```c
send_data_esp_now((const uint8_t *)"XXX:value", strlen("XXX:value"));
```

### 改为单播

1. 保存目标 MAC。
2. 调用 `esp_now_add_peer()` 添加目标。
3. 修改 `send_data_esp_now()`，把 `broadcast_mac` 替换为目标 MAC。

### 增加加密

需要配置 `peer_info.encrypt = true` 并设置 PMK/LMK。广播一般不适合加密，多设备群聊建议继续明文或改成多个单播 peer。
