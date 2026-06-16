/**
 * @file bb_display.h
 * @brief SSD1327 OLED 状态栏、动画和消息气泡接口。
 */

#ifndef BB_DISPLAY_H
#define BB_DISPLAY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 OLED 所在 SPI 总线、控制 GPIO 和 SSD1327。
 *
 * @retval None
 */
void setup_oled(void);

/**
 * @brief 绘制麦克风和音量状态图标。
 *
 * @retval None
 */
void draw_status(void);

/**
 * @brief 停止当前所有主界面动画。
 *
 * @retval None
 */
void stopAllAnimation(void);

/**
 * @brief OLED 主任务，负责开机动画、状态机和主界面刷新。
 *
 * @param arg FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void oled_task(void *arg);

/**
 * @brief OLED 顶部电池小图标刷新任务。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void batteryLevel_Task(void *pvParameters);

/**
 * @brief 播放关机 OLED 动画并进入深睡眠。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void byebye_anim(void *pvParameters);

/**
 * @brief 根据命令 ID 切换到对应命令动画。
 *
 * @param command_id MultiNet 命令 ID。
 * @retval None
 */
void bb_display_show_command_animation(int command_id);

/**
 * @brief 在 OLED 上显示一条文本气泡。
 *
 * @param text 待显示文本，函数内部会复制内容。
 * @retval None
 */
void bb_display_show_message_bubble(const char *text);

/**
 * @brief 清空 OLED 帧缓冲。
 *
 * @retval None
 */
void bb_display_clear(void);

/**
 * @brief 绘制充电模式的大电池图标。
 *
 * @param is_full true 表示已充满。
 * @param battery_level 1 到 4 的电量等级。
 * @param blink_state 充电闪烁状态。
 * @retval None
 */
void bb_display_draw_charging_battery(bool is_full, int battery_level, bool blink_state);

#ifdef __cplusplus
}
#endif

#endif /* BB_DISPLAY_H */
