# OLED 显示与动画

## 功能说明

OLED 使用 SSD1327，通过 SPI 驱动。显示内容包括：

- 开机动画。
- 主界面空闲/说话/接收状态动画。
- 麦克风和静音状态图标。
- 在线设备数量淡入。
- 命令动画。
- 文本消息气泡。
- 电池图标和充电大电池界面。
- 关机动画。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `bb_display.c` | OLED 初始化、绘制、动画任务、状态机。 |
| `bb_display.h` | 对外显示接口。 |
| `bb_animation.c` | 动画对象定义。 |
| `animation.h` | `spi_oled_animation_t` 结构和动画对象声明。 |
| `command_map.h` | 命令 ID 到动画对象映射。 |
| `components/esp32-spi-ssd1327` | SSD1327 驱动。 |

## 调用关系

```text
app_main()
  -> xTaskCreate(oled_task)

oled_task()
  -> setup_oled()
  -> draw_status()
  -> start animation_task()
  -> 根据 is_speaking / is_receiving / is_command 切换动画
```

命令动画：

```text
detect_Task() 或 handle_command_message()
  -> bb_display_show_command_animation()
  -> get_animation_by_key()
  -> stopAllAnimation()
  -> 创建 animation_task()
```

消息气泡：

```text
detect_Task() 或 handle_text_message()
  -> bb_display_show_message_bubble()
  -> bubble_text_task()
```

## 主要函数如何使用

### `setup_oled()`

用途：初始化 SPI 总线、OLED 控制 GPIO 和 SSD1327。

调用方式：

```c
setup_oled();
```

通常由 `oled_task()` 或充电模式 `charging_Task()` 调用。

### `oled_task(void *arg)`

用途：OLED 主任务，负责主界面状态刷新。

调用方式：

```c
xTaskCreatePinnedToCore(oled_task, "oled", 4 * 1024, NULL, 5, NULL, 0);
```

内部状态依据：

- `is_speaking == true`：说话动画。
- `is_receiving == true`：接收动画。
- 其它：空闲动画。
- `is_command == true`：命令动画模式。

### `draw_status()`

用途：刷新麦克风和音量图标。

调用方式：

```c
draw_status();
```

按键改变 `isMicOff` 或 `isMute` 后会调用它。

### `bb_display_show_command_animation(int command_id)`

用途：显示命令动画。

调用方式：

```c
bb_display_show_command_animation(command_id);
```

注意：

- 命令 ID 必须能在 `animation_map` 中找到。
- 函数会设置 `is_command = true`。
- 单击按键会退出命令动画模式。

### `bb_display_show_message_bubble(const char *text)`

用途：显示文本气泡。

调用方式：

```c
bb_display_show_message_bubble("message");
```

注意：传入文本会复制到内部缓冲，调用方不需要保持原字符串生命周期。

### `bb_display_draw_charging_battery()`

用途：绘制充电模式大电池图标。

调用方式：

```c
bb_display_draw_charging_battery(is_full, battery_level, blink_state);
```

参数：

- `is_full`：是否充满。
- `battery_level`：1 到 4。
- `blink_state`：充电时闪烁状态。

## 增加或修改功能

### 增加新动画

1. 准备帧数据。
2. 在 `bb_animation.c` 定义 `spi_oled_animation_t`。
3. 如果是命令动画，加入 `animation_map`。
4. 使用 `bb_display_show_command_animation()` 或自己创建 `animation_task()`。

### 修改状态界面

主界面状态切换在 `oled_task()` 中。新增状态建议：

1. 在 `app_state.h` 增加状态变量。
2. 在 `oled_task()` 中加入判断。
3. 调用 `stopAllAnimation()` 后启动新动画。

### 注意事项

- OLED 绘制共享 SPI，访问时要注意 `spi_mutex`。
- 动画任务使用 `task_handle` 保存任务句柄，切换动画前要停止旧任务。
- 不要在高频回调里大量刷新 OLED，容易影响音频实时性。

