# WS2812 状态灯

## 功能说明

本工程使用 1 颗 WS2812 作为状态灯，用不同颜色和呼吸效果表示设备状态：

- 空闲：默认青绿色。
- 正在接收：接收颜色呼吸。
- 正在说话：说话颜色呼吸。
- 充电中/充满：由 `charging_Task()` 直接设置橙色/绿色。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `bb_led.c` | WS2812 初始化、颜色设置、状态机和呼吸效果。 |
| `led.h` | 引脚、颜色和时间参数。 |
| `app_state.c/.h` | `is_speaking`、`is_receiving`、`isShutdown` 状态。 |
| `bb_power.c` | 充电模式下直接设置 LED 颜色。 |

## 调用关系

```text
app_main()
  -> ws2812_init()
  -> xTaskCreate(led_control_task)

led_control_task()
  -> 读取 is_speaking / is_receiving
  -> transition_to_color()
  -> breathing_multiplier()
  -> set_led_color()
```

充电模式：

```text
charging_Task()
  -> set_led_color(charging_color)
  -> set_led_color(charge_full_color)
```

## 主要函数如何使用

### `ws2812_init()`

用途：初始化 WS2812 RMT 驱动。

调用方式：

```c
ws2812_init();
```

必须在 `set_led_color()` 或 `led_control_task()` 前调用。

### `set_led_color(led_color_t color)`

用途：设置当前 LED RGB 颜色。

调用方式：

```c
led_color_t red = {255, 0, 0};
set_led_color(red);
```

注意：该函数直接刷新硬件。如果 `led_control_task()` 正在运行，它可能很快覆盖手动设置的颜色。

### `led_control_task(void *pvParameters)`

用途：根据全局状态自动刷新 LED。

调用方式：

```c
xTaskCreate(led_control_task, "led", 3 * 1024, NULL, 5, NULL);
```

状态优先级：

```text
is_speaking == true  -> STATE_SPEAKING
is_receiving == true -> STATE_RECEIVING
否则                 -> STATE_IDLE
```

任务会在 `isShutdown == true` 后退出循环。

## 配置参数

在 `led.h` 中：

```c
#define WS2812_GPIO_PIN 15
#define WS2812_LED_COUNT 1

#define DARKER_TEAL_R 32
#define DARKER_TEAL_G 150
#define DARKER_TEAL_B 32

#define RECEIVING_GREEN_R 32
#define RECEIVING_GREEN_G 32
#define RECEIVING_GREEN_B 255

#define SPEAKING_BLUE_R 255
#define SPEAKING_BLUE_G 30
#define SPEAKING_BLUE_B 0
```

注意：宏名里的颜色名称和实际 RGB 值可能不完全对应。修改颜色时以 RGB 值为准。

## 增加或修改功能

### 新增状态颜色

1. 在 `led.h` 增加颜色宏。
2. 在 `led_control_task()` 增加状态枚举。
3. 根据新的共享状态设置 `desired_state`。
4. 在 `switch` 中实现过渡和呼吸效果。

### 充电模式防止被覆盖

如果正常 LED 任务和充电任务同时运行，颜色会互相覆盖。充电模式建议不启动 `led_control_task()`，或增加一个 `isChargingMode` 状态让 LED 任务让出控制权。

