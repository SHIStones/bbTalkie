/**
 * @file app_state.c
 * @brief bbTalkie 的共享运行状态和硬件句柄定义。
 */

#include "include/app_state.h"
#include "include/fonts/fusion_pixel.h"
#include "include/fonts/fusion_pixel_30.h"

const char *TAG = "bbTalkie";

int macCount = 1;
int lastMacCount = 0;
mac_track_entry_t mac_track_list[MAX_MAC_TRACK] = {0};
uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const esp_afe_sr_iface_t *afe_handle = NULL;
srmodel_list_t *models = NULL;
StreamBufferHandle_t play_stream_buf = NULL;
QueueHandle_t s_recv_queue = NULL;

volatile bool is_receiving = false;
volatile bool is_speaking = false;
bool isShutdown = false;
bool isMicOff = false;
bool isMute = false;
bool is_command = false;
int state = 0;
int lastState = -1;

const variable_font_t font_10 = {
    .height = 10,
    .widths = font_10_widths,
    .offsets = font_10_offsets,
    .data = font_10_data,
};

const variable_font_t font_30 = {
    .height = 30,
    .widths = font_30_widths,
    .offsets = font_30_offsets,
    .data = font_30_data,
};

spi_device_handle_t oled_dev_handle = NULL;
struct spi_ssd1327 spi_ssd1327 = {
    .dc_pin_num = DC_PIN_NUM,
    .rst_pin_num = RST_PIN_NUM,
    .spi_handle = &oled_dev_handle,
};

SemaphoreHandle_t spi_mutex = NULL;

agc_t agc_custom = {
    .current_gain = 1.0f,
    .target_rms = TARGET_RMS,
    .attack_rate = AGC_ATTACK,
    .release_rate = AGC_RELEASE,
};
