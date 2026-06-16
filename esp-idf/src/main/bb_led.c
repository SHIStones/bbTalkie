/**
 * @file bb_led.c
 * @brief WS2812 状态灯初始化和状态颜色控制。
 */

#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#include "include/app_state.h"
#include "include/led.h"

static led_strip_handle_t led_strip = NULL;

static uint8_t lerp_color(uint8_t start, uint8_t end, float t)
{
    return (uint8_t)(start + t * (end - start));
}

static float breathing_multiplier(uint32_t time_ms)
{
    const float pi = 3.14159265358979323846f;
    float phase = (float)(time_ms % BREATHING_PERIOD_MS) / (float)BREATHING_PERIOD_MS;
    return 0.3f + 0.7f * (1.0f + sinf(2.0f * pi * phase)) / 2.0f;
}

void set_led_color(led_color_t color)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, color.r, color.g, color.b));
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void ws2812_init(void)
{
    ESP_LOGI(TAG, "Initializing WS2812 LED strip");

    led_strip_config_t strip_config = {
        .strip_gpio_num = WS2812_GPIO_PIN,
        .max_leds = WS2812_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}

static void transition_to_color(led_color_t from, led_color_t to)
{
    for (int step = 0; step <= TRANSITION_STEPS; step++)
    {
        float t = (float)step / (float)TRANSITION_STEPS;
        led_color_t current = {
            .r = lerp_color(from.r, to.r, t),
            .g = lerp_color(from.g, to.g, t),
            .b = lerp_color(from.b, to.b, t),
        };

        set_led_color(current);
        vTaskDelay(pdMS_TO_TICKS(TRANSITION_DELAY_MS));

        if (is_speaking == true)
        {
            break;
        }
    }
}

void led_control_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(TAG, "LED control task started");

    led_color_t darker_teal = {DARKER_TEAL_R, DARKER_TEAL_G, DARKER_TEAL_B};
    led_color_t receiving_green = {RECEIVING_GREEN_R, RECEIVING_GREEN_G, RECEIVING_GREEN_B};
    led_color_t speaking_blue = {SPEAKING_BLUE_R, SPEAKING_BLUE_G, SPEAKING_BLUE_B};

    typedef enum
    {
        STATE_IDLE,
        STATE_RECEIVING,
        STATE_SPEAKING,
        STATE_TRANSITION_TO_RECEIVING,
        STATE_TRANSITION_TO_IDLE,
        STATE_TRANSITION_TO_SPEAKING
    } led_state_t;

    led_state_t current_state = STATE_IDLE;
    led_color_t current_color = darker_teal;
    set_led_color(darker_teal);

    uint32_t breathing_start_time = 0;

    while (isShutdown == false)
    {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        led_state_t desired_state;

        if (is_speaking == true)
        {
            desired_state = STATE_SPEAKING;
        }
        else if (is_receiving == true)
        {
            desired_state = STATE_RECEIVING;
        }
        else
        {
            desired_state = STATE_IDLE;
        }

        if (desired_state != current_state)
        {
            switch (desired_state)
            {
            case STATE_SPEAKING:
                current_state = STATE_TRANSITION_TO_SPEAKING;
                breathing_start_time = current_time;
                break;
            case STATE_RECEIVING:
                current_state = STATE_TRANSITION_TO_RECEIVING;
                breathing_start_time = current_time;
                break;
            case STATE_IDLE:
                current_state = STATE_TRANSITION_TO_IDLE;
                break;
            default:
                break;
            }
        }

        switch (current_state)
        {
        case STATE_TRANSITION_TO_SPEAKING:
            transition_to_color(current_color, speaking_blue);
            current_color = speaking_blue;
            current_state = STATE_SPEAKING;
            break;
        case STATE_TRANSITION_TO_RECEIVING:
            transition_to_color(current_color, receiving_green);
            current_color = receiving_green;
            current_state = STATE_RECEIVING;
            break;
        case STATE_TRANSITION_TO_IDLE:
            transition_to_color(current_color, darker_teal);
            current_color = darker_teal;
            current_state = STATE_IDLE;
            break;
        case STATE_SPEAKING:
        {
            float multiplier = breathing_multiplier(current_time - breathing_start_time);
            led_color_t breathing_color = {
                .r = (uint8_t)(speaking_blue.r * multiplier),
                .g = (uint8_t)(speaking_blue.g * multiplier),
                .b = (uint8_t)(speaking_blue.b * multiplier),
            };
            set_led_color(breathing_color);
            vTaskDelay(pdMS_TO_TICKS(BREATHING_PERIOD_MS / BREATHING_STEPS));
            break;
        }
        case STATE_RECEIVING:
        {
            float multiplier = breathing_multiplier(current_time - breathing_start_time);
            led_color_t breathing_color = {
                .r = (uint8_t)(receiving_green.r * multiplier),
                .g = (uint8_t)(receiving_green.g * multiplier),
                .b = (uint8_t)(receiving_green.b * multiplier),
            };
            set_led_color(breathing_color);
            vTaskDelay(pdMS_TO_TICKS(BREATHING_PERIOD_MS / BREATHING_STEPS));
            break;
        }
        case STATE_IDLE:
        default:
            set_led_color(darker_teal);
            vTaskDelay(pdMS_TO_TICKS(50));
            break;
        }
    }

    vTaskDelete(NULL);
}
