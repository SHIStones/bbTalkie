/**
 * @file bb_display.c
 * @brief SSD1327 OLED 初始化、状态栏、电池图标和主界面动画任务。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "soc/adc_channel.h"

#include "include/animation.h"
#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_display.h"
#include "include/command_map.h"
#include "include/images/battery.h"
#include "include/images/logo.h"
#include "include/images/mic.h"
#include "include/images/volume.h"

extern const uint8_t text_bubble[11][47];

/**
 * @brief 逐帧播放一段 OLED 动画。
 *
 * @param pvParameters spi_oled_animation_t 指针。
 * @retval None
 */
static void animation_task(void *pvParameters)
{
    spi_oled_animation_t *anim_item = (spi_oled_animation_t *)pvParameters;

    if (anim_item == NULL)
    {
        printf("Invalid animation parameters\n");
        vTaskDelete(NULL);
        return;
    }

    int current_frame = 0;
    uint8_t bytes_per_row = (anim_item->width + 1) / 2;

    while (((anim_item->stop_frame == -1) && (anim_item->is_playing == true)) ||
           ((anim_item->stop_frame != -1) &&
            ((anim_item->is_playing == true) || (current_frame != anim_item->stop_frame))))
    {
        if ((anim_item->stop_frame == -1) && (anim_item->is_playing == false))
        {
            break;
        }

        if ((anim_item->stop_frame != -1) &&
            (anim_item->is_playing == false) &&
            ((state == 3) || (current_frame == anim_item->stop_frame)))
        {
            break;
        }

        const uint8_t *frame_data = anim_item->animation_data +
                                    (current_frame * anim_item->height * bytes_per_row);

        xSemaphoreTake(spi_mutex, portMAX_DELAY);
        spi_oled_drawImage(&spi_ssd1327,
                           anim_item->x,
                           anim_item->y,
                           anim_item->width,
                           anim_item->height,
                           frame_data,
                           SSD1327_GS_15);
        xSemaphoreGive(spi_mutex);

        if (anim_item->reverse == true)
        {
            current_frame--;
        }
        else
        {
            current_frame++;
        }

        if (current_frame >= anim_item->frame_count)
        {
            current_frame = 0;
        }
        else if (current_frame < 0)
        {
            current_frame = anim_item->frame_count - 1;
        }

        vTaskDelay(pdMS_TO_TICKS(anim_item->frame_delay_ms));
    }

    vTaskDelete(NULL);
}

/**
 * @brief 淡入绘制在线设备数量。
 *
 * @param arg 动态分配的字符串，任务退出前释放。
 * @retval None
 */
static void fade_in_drawCount(void *arg)
{
    char *input_text = (char *)arg;

    if ((input_text == NULL) || (strlen(input_text) == 0))
    {
        printf("Invalid text\n");
        free(input_text);
        vTaskDelete(NULL);
        return;
    }

    for (int i = 0; i < 15; i++)
    {
        if (state == 0)
        {
            spi_oled_drawText(&spi_ssd1327, 86, 46, &font_30, i / 2, input_text, 0);
            spi_oled_drawText(&spi_ssd1327, 85, 45, &font_30, i, input_text, 0);
        }
        else
        {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000 / 15));
    }

    free(input_text);
    vTaskDelete(NULL);
}

/**
 * @brief 绘制从顶部滑入的消息气泡。
 *
 * @param arg 动态分配的字符串，任务退出前释放。
 * @retval None
 */
static void bubble_text_task(void *arg)
{
    char *input_text = (char *)arg;

    if ((input_text == NULL) || (strlen(input_text) == 0))
    {
        printf("Invalid text for bubble task\n");
        free(input_text);
        vTaskDelete(NULL);
        return;
    }

    char text[64];
    strncpy(text, input_text, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';

    for (int i = -6; i < 0; i++)
    {
        spi_oled_drawImage(&spi_ssd1327, 17, i, 93, 11, (const uint8_t *)text_bubble, SSD1327_GS_15);
        spi_oled_drawText(&spi_ssd1327, 18, i, &font_10, SSD1327_GS_1, text, 86);
        vTaskDelay(pdMS_TO_TICKS(1000 / 15));
    }

    free(input_text);
    vTaskDelete(NULL);
}

/**
 * @brief 启动在线人数淡入显示任务。
 *
 * @param count 在线设备数量。
 * @retval None
 */
static void start_count_task(int count)
{
    char *mac_count_str = malloc(4);

    if (mac_count_str == NULL)
    {
        return;
    }

    snprintf(mac_count_str, 4, "%d", count);
    xTaskCreate(fade_in_drawCount, "drawCount", 2048, mac_count_str, 5, NULL);
}

void bb_display_show_command_animation(int command_id)
{
    if (anim_currentCommand != NULL)
    {
        anim_currentCommand->is_playing = false;
    }

    anim_currentCommand = get_animation_by_key(command_id);
    lastState = -1;
    is_command = true;
}

void bb_display_show_message_bubble(const char *text)
{
    if ((text == NULL) || (strlen(text) == 0))
    {
        return;
    }

    char *text_copy = malloc(strlen(text) + 1);
    if (text_copy == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for bubble text message");
        return;
    }

    strcpy(text_copy, text);
    xTaskCreate(bubble_text_task, "bubbleText", 4096, text_copy, 5, NULL);
}

void stopAllAnimation(void)
{
    anim_waveBar.is_playing = false;
    anim.is_playing = false;
    anim_speaking.is_playing = false;
    anim_receiving.is_playing = false;
    anim_idleWaveBar.is_playing = false;
    anim_idleBar.is_playing = false;
    anim_podcast.is_playing = false;
    anim_speaker.is_playing = false;

    if (anim_currentCommand != NULL)
    {
        anim_currentCommand->is_playing = false;
    }
}

void draw_status(void)
{
    if (isMicOff == false)
    {
        spi_oled_drawImage(&spi_ssd1327, 0, 0, 5, 10, (const uint8_t *)mic_high, SSD1327_GS_15);
    }
    else
    {
        spi_oled_drawImage(&spi_ssd1327, 0, 0, 5, 10, (const uint8_t *)mic_off, SSD1327_GS_15);
    }

    if (isMute == false)
    {
        spi_oled_drawImage(&spi_ssd1327, 6, 0, 9, 10, (const uint8_t *)volume_on, SSD1327_GS_15);
    }
    else
    {
        spi_oled_drawImage(&spi_ssd1327, 6, 0, 9, 10, (const uint8_t *)volume_off, SSD1327_GS_15);
    }
}

void setup_oled(void)
{
    spi_mutex = xSemaphoreCreateMutex();

    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = -1,
        .mosi_io_num = SPI_MOSI_PIN_NUM,
        .sclk_io_num = SPI_SCK_PIN_NUM,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = SPI_CS_PIN_NUM,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST_TAG, &dev_cfg, &oled_dev_handle));

    gpio_config_t dc_conf = {
        .pin_bit_mask = (1ULL << DC_PIN_NUM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    gpio_config(&dc_conf);

    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << RST_PIN_NUM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&rst_conf);

    spi_oled_init(&spi_ssd1327);
}

void bb_display_clear(void)
{
    spi_oled_framebuffer_clear(&spi_ssd1327, SSD1327_GS_0);
}

void bb_display_draw_charging_battery(bool is_full, int battery_level, bool blink_state)
{
    const uint8_t *icons[] = {
        (const uint8_t *)battery_large_1,
        (const uint8_t *)battery_large_2,
        (const uint8_t *)battery_large_3,
        (const uint8_t *)battery_large_4,
    };

    if (is_full == true)
    {
        spi_oled_drawImage(&spi_ssd1327, 32, 44, 64, 40, (const uint8_t *)battery_large_full, SSD1327_GS_15);
        return;
    }

    int show_level = (blink_state == true) ? (battery_level - 1) : (battery_level - 2);
    if (show_level < 0)
    {
        show_level = 0;
    }
    else if (show_level > 3)
    {
        show_level = 3;
    }

    spi_oled_drawImage(&spi_ssd1327, 32, 44, 64, 40, icons[show_level], SSD1327_GS_15);
}

void batteryLevel_Task(void *pvParameters)
{
    (void)pvParameters;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_GPIO7_CHANNEL, ADC_ATTEN_DB_6);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    int prev_gpio4 = -1;
    int prev_gpio5 = -1;
    int prev_battery_level = -1;
    uint32_t last_battery_check = 0;
    uint32_t last_blink = 0;
    bool blink_state = false;

    while (isShutdown == false)
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
        const uint8_t *icons[] = {
            (const uint8_t *)battery_1,
            (const uint8_t *)battery_2,
            (const uint8_t *)battery_3,
            (const uint8_t *)battery_4,
        };

        if (gpio5 == 0)
        {
            if (need_update == true)
            {
                spi_oled_drawImage(&spi_ssd1327, 112, 0, 16, 10, (const uint8_t *)battery_full, SSD1327_GS_15);
            }
        }
        else if (gpio4 == 0)
        {
            if (((now - last_blink) >= 500) || (need_update == true))
            {
                blink_state = !blink_state;
                int show_level = (blink_state == true) ? (battery_level - 1) : (battery_level - 2);
                if (show_level < 0)
                {
                    show_level = 0;
                }
                else if (show_level > 3)
                {
                    show_level = 3;
                }
                spi_oled_drawImage(&spi_ssd1327, 112, 0, 16, 10, icons[show_level], SSD1327_GS_15);
                last_blink = now;
            }
        }
        else if (need_update == true)
        {
            spi_oled_drawImage(&spi_ssd1327, 112, 0, 16, 10, icons[battery_level - 1], SSD1327_GS_15);
        }

        prev_gpio4 = gpio4;
        prev_gpio5 = gpio5;
        prev_battery_level = battery_level;

        vTaskDelay(pdMS_TO_TICKS(200));
    }

    vTaskDelete(NULL);
}

void oled_task(void *arg)
{
    (void)arg;

    setup_oled();
    printf("screen is on\n");

    spi_oled_framebuffer_clear(&spi_ssd1327, SSD1327_GS_0);
    for (size_t i = 32; i > 0; i--)
    {
        spi_oled_drawImage(&spi_ssd1327, 0, i, 128, 128, (const uint8_t *)logo, (32 - i) / 2);
        vTaskDelay(pdMS_TO_TICKS(1000 / 60));
    }

    printf("logo is painted\n");
    vTaskDelay(800 / portTICK_PERIOD_MS);
    spi_oled_framebuffer_clear(&spi_ssd1327, SSD1327_GS_0);
    spi_oled_drawText(&spi_ssd1327, 43, 0, &font_10, SSD1327_GS_5, "bbTalkie", 0);
    spi_oled_drawText(&spi_ssd1327, 44, 0, &font_10, SSD1327_GS_15, "bbTalkie", 0);
    draw_status();
    xTaskCreate(batteryLevel_Task, "battery", 4 * 1024, NULL, 5, NULL);

    bool isFirstBoot = true;

    while (isShutdown == false)
    {
        if (is_command == true)
        {
            state = 3;
        }
        else if (is_receiving == true)
        {
            state = 2;
        }
        else if (is_speaking == true)
        {
            state = 1;
        }
        else
        {
            state = 0;
        }

        int activeMacCount = 1;
        int64_t now = esp_timer_get_time() / 1000;
        for (int i = 0; i < MAX_MAC_TRACK; ++i)
        {
            if ((mac_track_list[i].valid == true) && (mac_track_list[i].last_seen_ms > (now - 5000)))
            {
                activeMacCount++;
            }
        }

        if ((state == 0) && (activeMacCount != lastMacCount))
        {
            lastMacCount = activeMacCount;
            spi_oled_draw_square(&spi_ssd1327, 74, 38, 36, 36, SSD1327_GS_0);
            start_count_task(activeMacCount);
        }

        if (state != lastState)
        {
            stopAllAnimation();
            spi_oled_draw_square(&spi_ssd1327, 0, 14, 128, 80, SSD1327_GS_0);
            lastState = state;

            switch (state)
            {
            case 0:
                anim.is_playing = true;
                lastMacCount = 0;
                xTaskCreate(animation_task, "idleSingleAnim", 2048, &anim, 5, &anim.task_handle);
                if (isFirstBoot == true)
                {
                    anim_idleBar.is_playing = true;
                    xTaskCreate(animation_task, "idleBarAnim", 2048, &anim_idleBar, 5, &anim_idleBar.task_handle);
                    isFirstBoot = false;
                }
                else
                {
                    anim_idleWaveBar.is_playing = true;
                    xTaskCreate(animation_task, "idleWaveBarAnim", 2048, &anim_idleWaveBar, 5, &anim_idleWaveBar.task_handle);
                }
                printf("Idle job create\n");
                break;

            case 1:
                anim_waveBar.reverse = false;
                anim_waveBar.is_playing = true;
                anim_podcast.is_playing = true;
                anim_speaking.is_playing = true;
                xTaskCreate(animation_task, "podcastAnim", 2048, &anim_podcast, 5, &anim_podcast.task_handle);
                xTaskCreate(animation_task, "waveBarAnim", 2048, &anim_waveBar, 5, &anim_waveBar.task_handle);
                xTaskCreate(animation_task, "speakingAnim", 2048, &anim_speaking, 5, &anim_speaking.task_handle);
                break;

            case 2:
                anim_waveBar.reverse = true;
                anim_waveBar.is_playing = true;
                anim_speaker.is_playing = true;
                anim_receiving.is_playing = true;
                xTaskCreate(animation_task, "speakerAnim", 2048, &anim_speaker, 5, &anim_speaker.task_handle);
                xTaskCreate(animation_task, "waveBarAnim", 2048, &anim_waveBar, 5, &anim_waveBar.task_handle);
                xTaskCreate(animation_task, "receivingAnim", 2048, &anim_receiving, 5, &anim_receiving.task_handle);
                break;

            case 3:
                printf("Create command anim task\n");
                spi_oled_draw_square(&spi_ssd1327, 0, 14, 128, 114, SSD1327_GS_0);
                if (anim_currentCommand == NULL)
                {
                    printf("No animation for this command\n");
                    is_command = false;
                    break;
                }
                anim_currentCommand->is_playing = true;
                xTaskCreate(animation_task, "customAnim", 2048, anim_currentCommand, 5, &anim_currentCommand->task_handle);
                break;

            default:
                break;
            }

            isFirstBoot = false;
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void byebye_anim(void *pvParameters)
{
    (void)pvParameters;

    printf("byebye_anim\n");
    spi_oled_animation_t *anim_item = &anim_byebye;
    int current_frame = 0;
    uint8_t bytes_per_row = (anim_item->width + 1) / 2;
    int loop_count = 0;

    for (int i = 0; i < (3 * anim_item->frame_count); i++)
    {
        const uint8_t *frame_data = anim_item->animation_data +
                                    (current_frame * anim_item->height * bytes_per_row);
        uint8_t gray_scale;

        if (loop_count < 2)
        {
            gray_scale = SSD1327_GS_15;
        }
        else
        {
            int fade_progress = anim_item->frame_count - current_frame - 1;
            gray_scale = (fade_progress * 15) / (anim_item->frame_count - 1);
            printf("Fade progress: %d, Gray scale: %d\n", fade_progress, gray_scale);
        }

        xSemaphoreTake(spi_mutex, portMAX_DELAY);
        spi_oled_drawImage(&spi_ssd1327,
                           anim_item->x,
                           anim_item->y,
                           anim_item->width,
                           anim_item->height,
                           frame_data,
                           gray_scale);
        xSemaphoreGive(spi_mutex);

        current_frame++;
        if (current_frame >= anim_item->frame_count)
        {
            current_frame = 0;
            loop_count++;
        }

        vTaskDelay(pdMS_TO_TICKS(anim_item->frame_delay_ms));
    }

    gpio_set_level(GPIO_NUM_3, 0);
    gpio_set_level(GPIO_NUM_9, 0);
    esp_deep_sleep_start();
    vTaskDelete(NULL);
}
