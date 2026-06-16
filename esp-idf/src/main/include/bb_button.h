/**
 * @file bb_button.h
 * @brief 单击、双击和长按按键事件接口。
 */

#ifndef BB_BUTTON_H
#define BB_BUTTON_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 GPIO 按键并注册单击、双击、长按回调。
 *
 * @retval ESP_OK 初始化成功。
 * @retval 其他 ESP-IDF 错误码表示初始化或回调注册失败。
 */
esp_err_t init_button(void);

#ifdef __cplusplus
}
#endif

#endif /* BB_BUTTON_H */
