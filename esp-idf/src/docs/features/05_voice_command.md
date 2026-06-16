# 语音命令识别

## 功能说明

本工程使用 ESP-SR 的 AFE + MultiNet 中文模型做语音命令识别。检测到命令后：

- 本机 OLED 播放对应命令动画。
- 通过 ESP-NOW 广播 `CMD:<id>`。
- 远端收到后同步播放同一个命令动画。

如果 MultiNet 超时但返回文本，则广播 `MSG:<text>`，远端显示消息气泡。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `main.c` | 初始化模型列表、AFE。 |
| `bb_audio.c` | `detect_Task()` 调用 MultiNet 检测。 |
| `bb_display.c` | 播放命令动画和消息气泡。 |
| `bb_animation.c` | 定义命令动画对象。 |
| `command_map.h` | 命令 ID 到动画对象的映射接口。 |
| `bb_radio.c` | 远端 `CMD:` / `MSG:` 消息解析。 |

## 调用关系

```text
app_main()
  -> models = esp_srmodel_init("model")
  -> afe_config_init()
  -> afe_handle->create_from_config()
  -> xTaskCreate(detect_Task)

detect_Task()
  -> esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE)
  -> multinet->create()
  -> afe_handle->fetch()
  -> multinet->detect()
  -> multinet->get_results()
  -> bb_display_show_command_animation()
  -> send_data_esp_now("CMD:<id>")
```

远端：

```text
esp_now_recv_cb()
  -> handle_command_message()
  -> bb_display_show_command_animation()
```

## 主要函数如何使用

### `detect_Task(void *arg)`

用途：处理 AFE 输出的语音数据，完成 VAD、音频发送和 MultiNet 命令识别。

调用方式：

```c
xTaskCreatePinnedToCore(detect_Task, "detect", 8 * 1024, (void *)afe_data, 5, NULL, 1);
```

关键流程：

```c
afe_fetch_result_t *res = afe_handle->fetch(afe_data);
mn_state = multinet->detect(model_data, res->data);
```

当 `mn_state == ESP_MN_STATE_DETECTED`：

```c
esp_mn_results_t *mn_result = multinet->get_results(model_data);
bb_display_show_command_animation(mn_result->command_id[0]);
send_data_esp_now((const uint8_t *)cmd_buffer, (size_t)cmd_len);
```

当 `mn_state == ESP_MN_STATE_TIMEOUT`：

```c
bb_display_show_message_bubble(mn_result->string);
send_data_esp_now((const uint8_t *)msg_buffer, (size_t)msg_len);
```

### `get_animation_by_key(int key)`

用途：根据 MultiNet 命令 ID 找到 OLED 动画。

调用方式：

```c
spi_oled_animation_t *animation = get_animation_by_key(command_id);
```

通常不用业务代码直接调用，`bb_display_show_command_animation()` 已封装。

### `bb_display_show_command_animation(int command_id)`

用途：切换到某个命令动画。

调用方式：

```c
bb_display_show_command_animation(mn_result->command_id[0]);
```

远端收到 `CMD:<id>` 后也调用它。

### `bb_display_show_message_bubble(const char *text)`

用途：显示一条文本气泡。

调用方式：

```c
bb_display_show_message_bubble("hello");
```

## 增加或修改命令

### 增加命令动画

1. 把动画数据加入 `include/images/custom.h` 或单独图片头文件。
2. 在 `bb_animation.c` 中新增一个 `spi_oled_animation_t`。
3. 在 `animation_map` 中添加命令 ID 到动画对象的映射。

示例：

```c
spi_oled_animation_t anim_new_cmd = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 20,
    .animation_data = (const uint8_t *)new_cmd,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};
```

### 调整识别语言或模型

当前使用：

```c
esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
```

如果换成其它语言或模型，需要确认 `model` 分区里已打包对应模型，并修改过滤条件。

### 避免命令识别影响对讲

`detect_Task()` 同时做音频发送和命令识别。如果后续命令识别占用过多 CPU，可以考虑：

- 降低 MultiNet 检测频率。
- 只在唤醒词后短时间识别命令。
- 把命令识别和音频发送拆成独立任务，用队列传递 AFE 输出。

