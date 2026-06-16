/**
 * @file bb_button.c
 * @brief 按键回调、单击双击长按和关机联动。
 */

#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "button_gpio.h"

#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_audio.h"
#include "include/bb_button.h"
#include "include/bb_display.h"

static button_handle_t btn = NULL;
static bool button_initialized = false;

static void cleanup_button(void)
{
    if ((btn != NULL) && (button_initialized == true))
    {
        iot_button_delete(btn);
        btn = NULL;
        button_initialized = false;
        printf("Button cleaned up\n");
    }
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    (void)arg;
    (void)usr_data;

    printf("Long press detected! Entering deep sleep mode...\n");
    isShutdown = true;
    stopAllAnimation();
    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreate(byebye_sound, "byebyeSound", 4 * 1024, NULL, 5, NULL);
    xTaskCreate(byebye_anim, "byebyeAnim", 4 * 1024, NULL, 5, NULL);
}

static void button_single_click_cb(void *arg, void *usr_data)
{
    (void)arg;
    (void)usr_data;

    printf("Single click\n");
    if (is_command == true)
    {
        is_command = false;
    }
    else
    {
        isMicOff = !isMicOff;
    }
    draw_status();
}

static void button_double_click_cb(void *arg, void *usr_data)
{
    (void)arg;
    (void)usr_data;

    printf("Double click\n");
    isMute = !isMute;
    draw_status();
}

esp_err_t init_button(void)
{
    cleanup_button();

    gpio_reset_pin(BUTTON_GPIO);
    vTaskDelay(pdMS_TO_TICKS(10));

    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON_GPIO,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = true,
    };

    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    if (ret != ESP_OK)
    {
        printf("Failed to create button device: %s\n", esp_err_to_name(ret));
        return ret;
    }

    ret = iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_long_press_cb, NULL);
    if (ret != ESP_OK)
    {
        printf("Failed to register long press callback\n");
        cleanup_button();
        return ret;
    }

    ret = iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL);
    if (ret != ESP_OK)
    {
        printf("Failed to register single click callback\n");
        cleanup_button();
        return ret;
    }

    ret = iot_button_register_cb(btn, BUTTON_DOUBLE_CLICK, NULL, button_double_click_cb, NULL);
    if (ret != ESP_OK)
    {
        printf("Failed to register double click callback\n");
        cleanup_button();
        return ret;
    }

    button_initialized = true;
    printf("Button initialized successfully\n");
    return ESP_OK;
}
