/**
 * @file main.c
 * @brief bbTalkie 应用入口，仅负责系统初始化和任务启动编排。
 */

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_init.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_audio.h"
#include "include/bb_button.h"
#include "include/bb_display.h"
#include "include/bb_power.h"
#include "include/bb_radio.h"
#include "include/led.h"

/**
 * @brief 配置深睡眠唤醒 GPIO，使充电状态和按键都能唤醒设备。
 *
 * @retval None
 */
static void configure_wakeup_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_WAKEUP_1) | (1ULL << GPIO_WAKEUP_2) | (1ULL << GPIO_WAKEUP_3),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    rtc_gpio_pullup_en(GPIO_WAKEUP_1);
    rtc_gpio_pullup_en(GPIO_WAKEUP_2);
    rtc_gpio_pullup_en(GPIO_WAKEUP_3);
    rtc_gpio_pulldown_dis(GPIO_WAKEUP_1);
    rtc_gpio_pulldown_dis(GPIO_WAKEUP_2);
    rtc_gpio_pulldown_dis(GPIO_WAKEUP_3);

    esp_sleep_enable_ext1_wakeup((1ULL << GPIO_WAKEUP_1) |
                                 (1ULL << GPIO_WAKEUP_2) |
                                 (1ULL << GPIO_WAKEUP_3),
                                 ESP_EXT1_WAKEUP_ANY_LOW);
}

/**
 * @brief 配置电源保持 GPIO，使主电源和外设电源保持开启。
 *
 * @retval None
 */
static void configure_power_gpio(void)
{
    gpio_config_t io_conf_3 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_3);

    gpio_config_t io_conf_9 = {
        .pin_bit_mask = (1ULL << GPIO_NUM_9),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_9);

    gpio_set_level(GPIO_NUM_3, 1);
    gpio_set_level(GPIO_NUM_9, 1);
}

/**
 * @brief 应用入口，完成外设初始化、语音模型创建和各 FreeRTOS 任务启动。
 *
 * @retval None
 */
void app_main(void)
{
    ws2812_init();

    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    isShutdown = false;

    esp_err_t ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    configure_power_gpio();
    configure_wakeup_gpio();

    if ((wakeup_pin_mask & (1ULL << GPIO_WAKEUP_1)) || (wakeup_pin_mask & (1ULL << GPIO_WAKEUP_3)))
    {
        printf("Wakeup caused by GPIO4 (Charger CHRG) / GPIO5 (Charger STDBY)\n");
        xTaskCreatePinnedToCore(charging_Task, "charging", 4 * 1024, NULL, 5, NULL, 0);
        return;
    }

    if ((wakeup_pin_mask & (1ULL << GPIO_WAKEUP_2)) != 0)
    {
        printf("Wakeup caused by GPIO8 (Button)\n");
    }

    while (gpio_get_level(BUTTON_GPIO) == 0)
    {
        printf("Waiting for button release\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (init_button() != ESP_OK)
    {
        printf("Failed to initialize button, retrying...\n");
        vTaskDelay(pdMS_TO_TICKS(500));
        if (init_button() != ESP_OK)
        {
            printf("Button initialization failed permanently\n");
        }
    }

    init_esp_now();

    ESP_ERROR_CHECK(esp_board_init(SAMPLE_RATE, 1, BIT_DEPTH));

    models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->vad_min_noise_ms = 800;
    afe_config->vad_min_speech_ms = 128;
    afe_config->vad_mode = VAD_MODE_1;
    afe_config->afe_linear_gain = 2.0;

    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);

    printf("afe_linear_gain:%f\n", afe_config->afe_linear_gain);
    printf("agc_init:%d, agc_mode:%d, agc_compression_gain_db:%d, agc_target_level_dbfs:%d\n",
           afe_config->agc_init,
           afe_config->agc_mode,
           afe_config->agc_compression_gain_db,
           afe_config->agc_target_level_dbfs);
    afe_config_free(afe_config);

    init_audio_stream_buffer();

    xTaskCreatePinnedToCore(oled_task, "oled", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(boot_sound, "bootSound", 3 * 1024, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(detect_Task, "detect", 8 * 1024, (void *)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(decode_Task, "decode", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(i2s_writer_task, "i2sWriter", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreate(ping_task, "ping", 3 * 1024, NULL, 5, NULL);
    xTaskCreate(led_control_task, "led_control", 3 * 1024, NULL, 5, NULL);
}
