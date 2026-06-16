/**
 * @file bb_radio.h
 * @brief ESP-NOW 无线收发、节点在线探测和接收队列接口。
 */

#ifndef BB_RADIO_H
#define BB_RADIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 Wi-Fi STA、ESP-NOW、广播 peer 和接收队列。
 *
 * @retval true 初始化成功。
 * @retval false 初始化失败，日志中会输出具体 ESP-IDF 错误。
 */
bool init_esp_now(void);

/**
 * @brief 通过 ESP-NOW 广播发送数据，超过单包长度时自动分片。
 *
 * @param data 待发送数据地址。
 * @param len 待发送数据字节数。
 * @retval None
 */
void send_data_esp_now(const uint8_t *data, size_t len);

/**
 * @brief 非阻塞读取一帧已接收的 ESP-NOW 音频数据。
 *
 * @param recv_data 输出接收数据结构。
 * @retval true 读取到一帧数据。
 * @retval false 当前无数据或队列未初始化。
 */
bool get_esp_now_data(esp_now_recv_data_t *recv_data);

/**
 * @brief 周期性广播 PING，用于其他设备统计在线节点数。
 *
 * @param arg FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void ping_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BB_RADIO_H */
