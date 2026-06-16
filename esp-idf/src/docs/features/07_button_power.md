# 按键、电源与充电

## 功能说明

本工程使用一个主按键实现：

- 单击：退出命令动画或切换麦克风开关。
- 双击：切换本地播放静音。
- 长按：播放关机音效和动画，然后进入关机流程。

充电模式会显示大电池界面，使用 GPIO 判断充电/充满状态，未充电时进入深睡眠。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `bb_button.c` | 按键初始化和回调。 |
| `bb_button.h` | 按键对外接口。 |
| `bb_power.c` | 充电模式、深睡眠、按键中断重启。 |
| `bb_power.h` | 电源对外接口。 |
| `main.c` | 上电检查 GPIO，决定正常启动或充电任务。 |
| `app_config.h` | 按键和唤醒 GPIO 宏。 |

## 调用关系

正常启动：

```text
app_main()
  -> configure_wakeup_gpio()
  -> configure_power_gpio()
  -> init_button()
```

按键事件：

```text
iot_button
  -> button_single_click_cb()
  -> button_double_click_cb()
  -> button_long_press_cb()
```

长按关机：

```text
button_long_press_cb()
  -> isShutdown = true
  -> stopAllAnimation()
  -> xTaskCreate(byebye_sound)
  -> xTaskCreate(byebye_anim)
```

充电模式：

```text
app_main()
  -> 判断唤醒/充电 GPIO
  -> xTaskCreate(charging_Task)

charging_Task()
  -> setup_oled()
  -> ADC 读电池电压
  -> GPIO4/GPIO5 判断充电/充满
  -> bb_display_draw_charging_battery()
  -> set_led_color()
  -> 未充电则 esp_deep_sleep_start()
```

## 主要函数如何使用

### `init_button()`

用途：初始化 GPIO 按键并注册单击、双击、长按回调。

调用方式：

```c
if (init_button() != ESP_OK)
{
    // 初始化失败，可重试或输出错误
}
```

依赖宏：

```c
#define BUTTON_GPIO 8
#define BUTTON_ACTIVE_LEVEL 0
#define LONG_PRESS_TIME_MS 2000
```

### `button_single_click_cb()`

用途：单击处理。

当前行为：

```text
如果正在显示命令动画：退出命令模式
否则：切换 isMicOff
最后刷新状态栏 draw_status()
```

扩展方式：如果要增加短按功能，在 `bb_button.c` 的该回调内处理。

### `button_double_click_cb()`

用途：双击处理。

当前行为：

```text
isMute = !isMute
draw_status()
```

### `button_long_press_cb()`

用途：长按关机。

当前行为：

```text
isShutdown = true
stopAllAnimation()
播放关机音效
播放关机动画
```

真正深睡眠动作在关机动画或电源流程中完成。

### `charging_Task(void *pvParameters)`

用途：充电界面和低功耗处理。

调用方式：

```c
xTaskCreate(charging_Task, "charging", 4 * 1024, NULL, 5, NULL);
```

当前 GPIO 判断：

- `GPIO_NUM_5 == 0`：充满。
- `GPIO_NUM_4 == 0`：充电中。
- 二者都不是：进入深睡眠。

## 增加或修改功能

### 增加按住说话 PTT

当前 `iot_button` 只注册了单击、双击、长按开始。如果要按住说话：

1. 在 `app_state.h` 增加 `extern volatile bool isPttPressed;`。
2. 在 `app_state.c` 定义。
3. 注册按下和释放事件。
4. 在 `detect_Task()` 发送条件中加入 `isPttPressed == true`。

### 修改充电检测

如果硬件 GPIO 含义变化，修改 `charging_Task()` 中 `gpio4` / `gpio5` 的判断。电池电压分档在同一函数中：

```c
if (voltage >= 4.0f) battery_level = 4;
else if (voltage >= 3.8f) battery_level = 3;
else if (voltage >= 3.6f) battery_level = 2;
else battery_level = 1;
```

