/**
 * @file animation.h
 * @brief OLED 主界面动画对象声明。
 */

#ifndef BBTALKIE_ANIMATION_H
#define BBTALKIE_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
    uint8_t frame_count;
    const uint8_t *animation_data;
    uint32_t frame_delay_ms;
    bool is_playing;
    int stop_frame;
    bool reverse;
    TaskHandle_t task_handle;
} spi_oled_animation_t;

extern spi_oled_animation_t anim;
extern spi_oled_animation_t anim_speaking;
extern spi_oled_animation_t anim_receiving;
extern spi_oled_animation_t anim_idleBar;
extern spi_oled_animation_t anim_waveBar;
extern spi_oled_animation_t anim_idleWaveBar;
extern spi_oled_animation_t anim_podcast;
extern spi_oled_animation_t anim_speaker;
extern spi_oled_animation_t anim_byebye;
extern spi_oled_animation_t *anim_currentCommand;

#ifdef __cplusplus
}
#endif

#endif /* BBTALKIE_ANIMATION_H */
