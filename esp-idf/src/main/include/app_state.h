/**
 * @file app_state.h
 * @brief 跨任务共享的运行状态和硬件句柄声明。
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "esp_now.h"
#include "esp32-spi-ssd1327.h"
#include "model_path.h"
#include "esp_afe_sr_models.h"

#include "app_config.h"
#include "agc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t mac[ESP_NOW_ETH_ALEN];
    int64_t last_seen_ms;
    bool valid;
} mac_track_entry_t;

typedef struct
{
    uint8_t data[ESP_NOW_MAX_DATA_LEN_V2];
    size_t data_len;
} esp_now_recv_data_t;

typedef struct
{
    int16_t buffer[ADPCM_FRAME_SIZE];
    size_t current_samples;
} adpcm_encode_buffer_t;

extern const char *TAG;
extern int macCount;
extern int lastMacCount;
extern mac_track_entry_t mac_track_list[MAX_MAC_TRACK];
extern uint8_t broadcast_mac[ESP_NOW_ETH_ALEN];

extern const esp_afe_sr_iface_t *afe_handle;
extern srmodel_list_t *models;
extern StreamBufferHandle_t play_stream_buf;
extern QueueHandle_t s_recv_queue;

extern volatile bool is_receiving;
extern volatile bool is_speaking;
extern bool isShutdown;
extern bool isMicOff;
extern bool isMute;
extern bool is_command;
extern int state;
extern int lastState;

extern const variable_font_t font_10;
extern const variable_font_t font_30;
extern spi_device_handle_t oled_dev_handle;
extern struct spi_ssd1327 spi_ssd1327;
extern SemaphoreHandle_t spi_mutex;
extern agc_t agc_custom;

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
