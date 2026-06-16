/**
 * @file app_config.h
 * @brief bbTalkie 固件的硬件引脚、音频帧和通信参数配置。
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLE_RATE 16000
#define BIT_DEPTH 16

#define ENCODED_BUF_SIZE 10240
#define PLAY_RING_BUFFER_SIZE 1024
#define PLAY_CHUNK_SIZE 512

#define ESP_NOW_PACKET_SIZE 512
#define PING_MAGIC "PING"
#define PING_MAGIC_LEN 4
#define MAX_MAC_TRACK 9
#define MAC_TIMEOUT_MS 20000

#define ADPCM_FRAME_SIZE 505
#define ADPCM_FRAME_BYTES (ADPCM_FRAME_SIZE * sizeof(int16_t))

#define SPI_MOSI_PIN_NUM 14
#define SPI_SCK_PIN_NUM 13
#define SPI_CS_PIN_NUM 10
#define DC_PIN_NUM 12
#define RST_PIN_NUM 11
#define SPI_HOST_TAG SPI2_HOST

#define GPIO_WAKEUP_1 GPIO_NUM_4
#define GPIO_WAKEUP_2 GPIO_NUM_8
#define GPIO_WAKEUP_3 GPIO_NUM_5

#define BUTTON_GPIO 8
#define BUTTON_ACTIVE_LEVEL 0
#define LONG_PRESS_TIME_MS 2000

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
