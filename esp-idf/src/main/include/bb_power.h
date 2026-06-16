/**
 * @file bb_power.h
 * @brief 充电唤醒界面、低功耗唤醒和深睡眠相关任务接口。
 */

#ifndef BB_POWER_H
#define BB_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 充电唤醒后的显示和 LED 状态任务。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void charging_Task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* BB_POWER_H */
