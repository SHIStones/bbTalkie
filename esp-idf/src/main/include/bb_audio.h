/**
 * @file bb_audio.h
 * @brief 麦克风采集、语音识别、ADPCM 编解码和扬声器播放接口。
 */

#ifndef BB_AUDIO_H
#define BB_AUDIO_H

#include <stddef.h>
#include <stdint.h>

#include "agc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 计算一段 16 位 PCM 音频的 RMS 音量。
 *
 * @param buffer PCM 样本缓冲区。
 * @param samples 样本数量。
 * @retval float RMS 音量值。
 */
float calculate_rms(int16_t *buffer, size_t samples);

/**
 * @brief 对播放音频应用自动增益控制。
 *
 * @param buffer PCM 样本缓冲区，函数会原地修改。
 * @param samples 样本数量。
 * @param agc AGC 状态和参数。
 * @retval None
 */
void apply_agc(int16_t *buffer, size_t samples, agc_t *agc);

/**
 * @brief 从 I2S 麦克风读取数据并喂给 ESP-SR AFE。
 *
 * @param arg esp_afe_sr_data_t 指针。
 * @retval None
 */
void feed_Task(void *arg);

/**
 * @brief 将 PCM 编码为 ADPCM。
 *
 * @param pcm_data 输入 PCM 数据。
 * @param pcm_len_bytes PCM 字节数。
 * @param adpcm_output 输出 ADPCM 缓冲区。
 * @param adpcm_len 输出 ADPCM 字节数。
 * @retval None
 */
void encode_adpcm(const int16_t *pcm_data, size_t pcm_len_bytes, uint8_t *adpcm_output, size_t *adpcm_len);

/**
 * @brief 将 ADPCM 解码为 PCM。
 *
 * @param adpcm_data 输入 ADPCM 数据。
 * @param adpcm_len ADPCM 字节数。
 * @param pcm_output 输出 PCM 缓冲区。
 * @param pcm_len 输出 PCM 字节数。
 * @retval None
 */
void decode_adpcm(const uint8_t *adpcm_data, size_t adpcm_len, uint8_t *pcm_output, size_t *pcm_len);

/**
 * @brief 接收无线音频帧、解码后写入播放流缓冲区。
 *
 * @param arg FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void decode_Task(void *arg);

/**
 * @brief 从 AFE 获取语音片段，完成本地命令识别并广播语音或命令。
 *
 * @param arg esp_afe_sr_data_t 指针。
 * @retval None
 */
void detect_Task(void *arg);

/**
 * @brief 播放开机提示音。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void boot_sound(void *pvParameters);

/**
 * @brief 播放关机提示音。
 *
 * @param pvParameters FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void byebye_sound(void *pvParameters);

/**
 * @brief 从播放流缓冲区取 PCM 数据并写入 I2S 输出。
 *
 * @param arg FreeRTOS 任务参数，当前未使用。
 * @retval None
 */
void i2s_writer_task(void *arg);

/**
 * @brief 创建播放用 StreamBuffer。
 *
 * @retval None
 */
void init_audio_stream_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* BB_AUDIO_H */
