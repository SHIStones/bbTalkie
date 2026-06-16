/**
 * @file command_map.h
 * @brief 语音命令 ID 到 OLED 自定义动画的映射接口。
 */

#ifndef BBTALKIE_COMMAND_MAP_H
#define BBTALKIE_COMMAND_MAP_H

#include "animation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int key;
    spi_oled_animation_t *animation;
} animation_map_entry_t;

extern spi_oled_animation_t anim_turn_left;
extern spi_oled_animation_t anim_turn_right;
extern spi_oled_animation_t anim_go_straight;
extern spi_oled_animation_t anim_time_out;
extern spi_oled_animation_t anim_wait;
extern spi_oled_animation_t anim_hello;
extern spi_oled_animation_t anim_check_mark;
extern spi_oled_animation_t anim_help_sos;
extern spi_oled_animation_t anim_up_hill;
extern spi_oled_animation_t anim_down_hill;
extern spi_oled_animation_t anim_slow;
extern spi_oled_animation_t anim_attention;
extern spi_oled_animation_t anim_walk;
extern spi_oled_animation_t anim_add_oil;
extern spi_oled_animation_t anim_eat;
extern spi_oled_animation_t anim_drink;

/**
 * @brief 根据语音命令 ID 查找对应 OLED 动画。
 *
 * @param key MultiNet 输出的命令 ID。
 * @retval spi_oled_animation_t* 找到时返回动画对象。
 * @retval NULL 未找到匹配动画。
 */
spi_oled_animation_t *get_animation_by_key(int key);

#ifdef __cplusplus
}
#endif

#endif /* BBTALKIE_COMMAND_MAP_H */
