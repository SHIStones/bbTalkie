/**
 * @file bb_power.c
 * @brief 充电状态检测和低功耗处理。
 */

#include <stdio.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/adc_channel.h"

#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_display.h"
#include "include/bb_power.h"
#include "include/led.h"

static void IRAM_ATTR button_isr_handler(void *arg)
{
    (void)arg;
    esp_restart();
}

void charging_Task(void *pvParameters)
{
    (void)pvParameters;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_WAKEUP_2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_WAKEUP_2, button_isr_handler, NULL);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_GPIO7_CHANNEL, ADC_ATTEN_DB_6);

    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf2);

    int prev_gpio4 = -1;
    int prev_gpio5 = -1;
    int prev_battery_level = -1;
    uint32_t last_battery_check = 0;
    uint32_t last_blink = 0;
    bool blink_state = false;

    setup_oled();
    bb_display_clear();

    while (1)
    {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        int gpio4 = gpio_get_level(GPIO_NUM_4);
        int gpio5 = gpio_get_level(GPIO_NUM_5);
        bool gpio_changed = ((gpio4 != prev_gpio4) || (gpio5 != prev_gpio5));
        int battery_level = prev_battery_level;

        if (((now - last_battery_check) >= 60000) || (prev_battery_level == -1))
        {
            int adc_raw = adc1_get_raw(ADC1_GPIO7_CHANNEL);
            float voltage = (adc_raw * 2.2f / 4095.0f) * 2.0f;

            if (voltage >= 4.0f)
            {
                battery_level = 4;
            }
            else if (voltage >= 3.8f)
            {
                battery_level = 3;
            }
            else if (voltage >= 3.6f)
            {
                battery_level = 2;
            }
            else
            {
                battery_level = 1;
            }

            last_battery_check = now;
        }

        bool need_update = (gpio_changed || (battery_level != prev_battery_level));

        if (gpio5 == 0)
        {
            if (need_update == true)
            {
                bb_display_draw_charging_battery(true, battery_level, blink_state);
                led_color_t charge_full_color = {0, 255, 0};
                set_led_color(charge_full_color);
            }
        }
        else if (gpio4 == 0)
        {
            if (need_update == true)
            {
                led_color_t charging_color = {240, 150, 0};
                set_led_color(charging_color);
            }

            if (((now - last_blink) >= 500) || (need_update == true))
            {
                blink_state = !blink_state;
                bb_display_draw_charging_battery(false, battery_level, blink_state);
                last_blink = now;
            }
        }
        else
        {
            printf("not charging\n");
            gpio_set_level(GPIO_NUM_3, 0);
            gpio_set_level(GPIO_NUM_9, 0);
            esp_deep_sleep_start();
            vTaskDelete(NULL);
        }

        prev_gpio4 = gpio4;
        prev_gpio5 = gpio5;
        prev_battery_level = battery_level;

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
