/**
 * @file led.h
 * @brief WS2812 状态灯颜色配置和控制接口。
 */

#ifndef BBTALKIE_LED_H
#define BBTALKIE_LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#define TRANSITION_STEPS 10
#define TRANSITION_DELAY_MS 20
#define BREATHING_PERIOD_MS 1000
#define BREATHING_STEPS 50

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_color_t;

/**
 * @brief 初始化 WS2812 灯带驱动。
 *
 * @retval None
 */
void ws2812_init(void);

/**
 * @brief 设置 WS2812 的 RGB 颜色。
 *
 * @param color 目标颜色。
 * @retval None
 */
void set_led_color(led_color_t color);

/**
 * @brief 根据说话、接收和空闲状态刷新 WS2812 颜色。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void led_control_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* BBTALKIE_LED_H */
