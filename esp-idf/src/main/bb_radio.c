/**
 * @file bb_radio.c
 * @brief ESP-NOW 广播音频、命令消息和在线节点维护。
 */

#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "include/app_config.h"
#include "include/app_state.h"
#include "include/bb_display.h"
#include "include/bb_radio.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
/**
 * @brief ESP-NOW 发送完成回调，记录发送失败事件。
 *
 * @param tx_info ESP-IDF 5.5 及以上版本的发送信息。
 * @param status 发送状态。
 * @retval None
 */
static void esp_now_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
#else
/**
 * @brief ESP-NOW 发送完成回调，记录发送失败事件。
 *
 * @param mac_addr 目标 MAC 地址。
 * @param status 发送状态。
 * @retval None
 */
static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
#endif
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
    (void)tx_info;
#else
    (void)mac_addr;
#endif

    if (status != ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGW(TAG, "ESP-NOW data send failed");
    }
}

void send_data_esp_now(const uint8_t *data, size_t len)
{
    size_t offset = 0;

    if ((data == NULL) || (len == 0))
    {
        return;
    }

    while (offset < len)
    {
        size_t chunk_size = ((len - offset) > ESP_NOW_PACKET_SIZE) ? ESP_NOW_PACKET_SIZE : (len - offset);
        esp_err_t ret = esp_now_send(broadcast_mac, data + offset, chunk_size);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW send failed at offset %zu: %s", offset, esp_err_to_name(ret));
        }

        offset += chunk_size;
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

bool get_esp_now_data(esp_now_recv_data_t *recv_data)
{
    if ((s_recv_queue == NULL) || (recv_data == NULL))
    {
        return false;
    }

    return (xQueueReceive(s_recv_queue, recv_data, 0) == pdTRUE);
}

/**
 * @brief 更新 PING 发送方的在线时间，必要时新增在线设备。
 *
 * @param mac 发送方 MAC 地址。
 * @retval None
 */
static void update_mac_track_list(const uint8_t *mac)
{
    int64_t now = esp_timer_get_time() / 1000;
    bool found = false;

    for (int i = 0; i < MAX_MAC_TRACK; ++i)
    {
        if ((mac_track_list[i].valid == true) &&
            (memcmp(mac_track_list[i].mac, mac, ESP_NOW_ETH_ALEN) == 0))
        {
            mac_track_list[i].last_seen_ms = now;
            found = true;
            break;
        }
    }

    if (found == false)
    {
        for (int i = 0; i < MAX_MAC_TRACK; ++i)
        {
            if (mac_track_list[i].valid == false)
            {
                memcpy(mac_track_list[i].mac, mac, ESP_NOW_ETH_ALEN);
                mac_track_list[i].last_seen_ms = now;
                mac_track_list[i].valid = true;
                macCount += 1;
                ESP_LOGI(TAG, "Added MAC");
                break;
            }
        }
    }
}

/**
 * @brief 处理远端命令消息并切换本机命令动画。
 *
 * @param data ESP-NOW 原始数据。
 * @param data_len 原始数据长度。
 * @retval None
 */
static void handle_command_message(const uint8_t *data, int data_len)
{
    char cmd_param[32];
    int param_len = data_len - 4;

    if (param_len >= (int)sizeof(cmd_param))
    {
        param_len = (int)sizeof(cmd_param) - 1;
    }

    memcpy(cmd_param, data + 4, (size_t)param_len);
    cmd_param[param_len] = '\0';

    int cmd_value = atoi(cmd_param);
    ESP_LOGI(TAG, "Processed CMD: %d", cmd_value);
    bb_display_show_command_animation(cmd_value);
}

/**
 * @brief 处理远端文本消息并在 OLED 上显示气泡。
 *
 * @param data ESP-NOW 原始数据。
 * @param data_len 原始数据长度。
 * @retval None
 */
static void handle_text_message(const uint8_t *data, int data_len)
{
    char msg_content[ESP_NOW_MAX_DATA_LEN_V2];
    int content_len = data_len - 4;

    if (content_len >= (int)sizeof(msg_content))
    {
        content_len = (int)sizeof(msg_content) - 1;
    }

    memcpy(msg_content, data + 4, (size_t)content_len);
    msg_content[content_len] = '\0';

    ESP_LOGI(TAG, "Processed MSG: %s", msg_content);
    bb_display_show_message_bubble(msg_content);
}

/**
 * @brief ESP-NOW 接收回调，区分 PING、CMD、MSG 和音频帧。
 *
 * @param recv_info 接收包元信息。
 * @param data 数据地址。
 * @param data_len 数据长度。
 * @retval None
 */
static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    if ((recv_info == NULL) || (data == NULL) || (data_len <= 0))
    {
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(0));
    ESP_LOGI(TAG, "Received %d bytes, RSSI: %d dBm", data_len, recv_info->rx_ctrl->rssi);

    if ((data_len == PING_MAGIC_LEN) && (memcmp(data, PING_MAGIC, PING_MAGIC_LEN) == 0))
    {
        ESP_LOGI(TAG, "Received PING from %02x:%02x:%02x:%02x:%02x:%02x",
                 recv_info->src_addr[0], recv_info->src_addr[1],
                 recv_info->src_addr[2], recv_info->src_addr[3],
                 recv_info->src_addr[4], recv_info->src_addr[5]);
        update_mac_track_list(recv_info->src_addr);
    }
    else if ((data_len >= 5) && (memcmp(data, "CMD:", 4) == 0))
    {
        handle_command_message(data, data_len);
    }
    else if ((data_len >= 5) && (memcmp(data, "MSG:", 4) == 0))
    {
        handle_text_message(data, data_len);
    }
    else if (s_recv_queue != NULL)
    {
        esp_now_recv_data_t recv_data;
        size_t copy_len = ((size_t)data_len > ESP_NOW_MAX_DATA_LEN_V2) ? ESP_NOW_MAX_DATA_LEN_V2 : (size_t)data_len;

        is_receiving = true;
        memcpy(recv_data.data, data, copy_len);
        recv_data.data_len = copy_len;
        (void)xQueueSend(s_recv_queue, &recv_data, 0);
    }
}

bool init_esp_now(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi set mode failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_now_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return false;
    }

    esp_now_register_send_cb(esp_now_send_cb);
    esp_now_register_recv_cb(esp_now_recv_cb);

    esp_now_peer_info_t peer_info = {0};
    memcpy(peer_info.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    peer_info.channel = 0;
    peer_info.encrypt = false;

    ret = esp_now_add_peer(&peer_info);
    if ((ret != ESP_OK) && (ret != ESP_ERR_ESPNOW_EXIST))
    {
        ESP_LOGE(TAG, "Add broadcast peer failed: %s", esp_err_to_name(ret));
        return false;
    }

    ESP_ERROR_CHECK(esp_now_set_wake_window(25));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(100));

    s_recv_queue = xQueueCreate(10, sizeof(esp_now_recv_data_t));
    if (s_recv_queue == NULL)
    {
        ESP_LOGE(TAG, "ESP-NOW receive queue create failed");
        return false;
    }

    ESP_LOGI(TAG, "ESP-NOW initialized successfully");
    return true;
}

void ping_task(void *arg)
{
    (void)arg;
    send_data_esp_now((const uint8_t *)PING_MAGIC, PING_MAGIC_LEN);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        send_data_esp_now((const uint8_t *)PING_MAGIC, PING_MAGIC_LEN);
    }
}
