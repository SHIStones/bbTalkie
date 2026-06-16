# 回声消除

## 功能说明

本工程没有自己手写 AEC 算法。回声消除由 ESP-SR AFE 内部完成，工程负责把“播放参考信号”和“麦克风信号”按正确通道喂给 AFE。

结合原理图，硬件使用 I2S 数字 loopback：

```text
ESP32 IO18 I2S_DOUT
  -> I2S AMP DIN -> 功放 -> 喇叭
  -> R4 5.1K -> ESP32 IO17 I2S_DIN 左声道参考信号

I2S MIC SD -> ESP32 IO17 I2S_DIN 右声道麦克风信号
```

AFE 输入格式当前为 `"RM"`：

```text
R = Reference，播放参考信号
M = Microphone，麦克风信号
```

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `main.c` | 创建 AFE，传入 `esp_get_input_format()`。 |
| `bb_audio.c` | `feed_Task()` 把 I2S 输入喂给 AFE。 |
| `components/hardware_driver/boards/esp32s3-bbtalkie/bsp_board.c` | I2S 初始化、读取双通道数据、返回输入格式。 |
| `components/hardware_driver/boards/include/esp32_s3_bbtalkie_board.h` | I2S 引脚定义。 |

## 调用关系

```text
app_main()
  -> esp_board_init()
  -> afe_config_init(esp_get_input_format(), ...)
  -> afe_handle->create_from_config()
  -> xTaskCreate(feed_Task)

feed_Task()
  -> esp_get_feed_channel()
  -> esp_get_feed_data()
  -> afe_handle->feed()

esp_get_feed_data()
  -> bsp_get_feed_data()
  -> i2s_channel_read(rx_handle)
  -> 转换为 int16_t 双通道 buffer
```

## 主要函数如何使用

### `bsp_get_input_format()`

用途：告诉 ESP-SR AFE 输入通道顺序。

当前实现：

```c
char *bsp_get_input_format(void)
{
    return "RM";
}
```

使用位置：

```c
afe_config_t *afe_config = afe_config_init(esp_get_input_format(),
                                           models,
                                           AFE_TYPE_SR,
                                           AFE_MODE_LOW_COST);
```

修改原则：

- 如果输入 buffer 顺序是 `reference, mic`，使用 `"RM"`。
- 如果输入 buffer 顺序是 `mic, reference`，使用 `"MR"`。
- 如果没有 reference 通道，AEC 无法正常工作，应改为单麦输入格式并关闭相关预期。

### `bsp_get_feed_channel()`

用途：返回喂给 AFE 的通道数量。

当前实现：

```c
int bsp_get_feed_channel(void)
{
    return 2;
}
```

AFE 创建后，`feed_Task()` 会校验：

```c
assert(nch == feed_channel);
```

如果输入格式从 `"RM"` 改成单通道，通道数也必须同步修改。

### `bsp_get_feed_data()`

用途：从 I2S RX 读取 32 bit stereo 数据，转换为 AFE 需要的 16 bit 双通道数据。

当前输出逻辑：

```c
for (int i = 0; i < samples_read; i += 2)
{
    buffer[i] = (int16_t)(i2s_read_buffer[i] >> 14);
    buffer[i + 1] = (int16_t)(i2s_read_buffer[i + 1] >> 16);
}
```

结合原理图，应理解为：

```text
buffer[0] = reference0
buffer[1] = mic0
buffer[2] = reference1
buffer[3] = mic1
...
```

注意：源码当前注释写成“左声道 mic、右声道 loopback”，与原理图和 `"RM"` 不一致，后续建议修正注释。

### `feed_Task()`

用途：把双通道音频持续送入 AFE。

调用方式：

```c
xTaskCreatePinnedToCore(feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
```

## AEC 生效条件

1. `IO18` 播放数据确实通过 R4 回到 `IO17`。
2. 麦克风只在右声道时隙输出，左声道时隙允许 loopback 输入。
3. `bsp_get_feed_data()` 的输出顺序和 `"RM"` 一致。
4. 播放端输出到左声道，和 loopback 的左声道匹配。
5. AFE 配置包含 reference 通道。

## 调试建议

### 验证通道顺序

可以临时在 `bsp_get_feed_data()` 中分别统计左/右声道 RMS：

```c
left = i2s_read_buffer[i];
right = i2s_read_buffer[i + 1];
```

播放提示音但不说话时：

- reference 通道应有明显波形。
- mic 通道可能有较小声学回声。

对着麦克风说话但不播放时：

- mic 通道应明显变化。
- reference 通道不应随说话明显变化。

### 调整位移

当前 reference 使用 `>> 14`，mic 使用 `>> 16`。这会影响两个通道的幅度。AEC 对参考信号幅度比较敏感，若消回声效果差，应先确认通道顺序，再微调位移或增益。

## 修改入口

- 改 AFE 通道格式：`bsp_get_input_format()`。
- 改 I2S 输入转换：`bsp_get_feed_data()`。
- 改播放声道：`bsp_audio_play()`。
- 改 AFE 参数：`main.c` 中 `afe_config` 字段。

