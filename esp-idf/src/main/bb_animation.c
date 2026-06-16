/**
 * @file bb_animation.c
 * @brief OLED 主界面动画对象和命令动画映射。
 */

#include <stdio.h>

#include "include/app_state.h"
#include "include/animation.h"
#include "include/command_map.h"
#include "include/images/custom.h"
#include "include/images/home.h"

spi_oled_animation_t *anim_currentCommand = NULL;

spi_oled_animation_t anim = {
    .x = 10,
    .y = 35,
    .width = 54,
    .height = 41,
    .frame_count = 14,
    .animation_data = (const uint8_t *)idle_single,
    .frame_delay_ms = 1000 / 5,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_speaking = {
    .x = 10,
    .y = 35,
    .width = 54,
    .height = 41,
    .frame_count = 7,
    .animation_data = (const uint8_t *)speaking_single,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_receiving = {
    .x = 10,
    .y = 35,
    .width = 54,
    .height = 41,
    .frame_count = 6,
    .animation_data = (const uint8_t *)receiving_single,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_idleBar = {
    .x = 10,
    .y = 95,
    .width = 101,
    .height = 12,
    .frame_count = 14,
    .animation_data = (const uint8_t *)idle_bar,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_waveBar = {
    .x = 1,
    .y = 85,
    .width = 126,
    .height = 40,
    .frame_count = 29,
    .animation_data = (const uint8_t *)wave_bar,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_idleWaveBar = {
    .x = 1,
    .y = 85,
    .width = 126,
    .height = 40,
    .frame_count = 36,
    .animation_data = (const uint8_t *)idle_wave_bar,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_podcast = {
    .x = 74,
    .y = 38,
    .width = 36,
    .height = 36,
    .frame_count = 24,
    .animation_data = (const uint8_t *)podcast,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_speaker = {
    .x = 74,
    .y = 38,
    .width = 36,
    .height = 36,
    .frame_count = 46,
    .animation_data = (const uint8_t *)speaker,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_byebye = {
    .x = 0,
    .y = 0,
    .width = 128,
    .height = 128,
    .frame_count = 7,
    .animation_data = (const uint8_t *)byebye_image,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_turn_left = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 22,
    .animation_data = (const uint8_t *)turn_left,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_turn_right = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 22,
    .animation_data = (const uint8_t *)turn_right,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_go_straight = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 22,
    .animation_data = (const uint8_t *)go_straight,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_time_out = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 67,
    .animation_data = (const uint8_t *)time_out,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_wait = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 10,
    .animation_data = (const uint8_t *)wait,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_hello = {
    .x = 8,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 32,
    .animation_data = (const uint8_t *)wave,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_check_mark = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 32,
    .animation_data = (const uint8_t *)check_mark,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_help_sos = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 5,
    .animation_data = (const uint8_t *)help_sos,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_up_hill = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 30,
    .animation_data = (const uint8_t *)up_hill,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_down_hill = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 30,
    .animation_data = (const uint8_t *)down_hill,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_slow = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 30,
    .animation_data = (const uint8_t *)slow,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_attention = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 30,
    .animation_data = (const uint8_t *)attention,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_walk = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 8,
    .animation_data = (const uint8_t *)walk,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_add_oil = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 57,
    .animation_data = (const uint8_t *)add_oil,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_eat = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 60,
    .animation_data = (const uint8_t *)eat,
    .frame_delay_ms = 1000 / 30,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

spi_oled_animation_t anim_drink = {
    .x = 14,
    .y = 14,
    .width = 100,
    .height = 100,
    .frame_count = 8,
    .animation_data = (const uint8_t *)drink,
    .frame_delay_ms = 1000 / 15,
    .stop_frame = -1,
    .reverse = false,
    .task_handle = NULL,
};

static animation_map_entry_t animation_map[] = {
    {1, &anim_turn_left},
    {2, &anim_turn_right},
    {3, &anim_go_straight},
    {4, &anim_time_out},
    {5, &anim_wait},
    {6, &anim_hello},
    {7, &anim_check_mark},
    {8, &anim_help_sos},
    {9, &anim_up_hill},
    {10, &anim_down_hill},
    {11, &anim_slow},
    {12, &anim_attention},
    {13, &anim_walk},
    {14, &anim_eat},
    {15, &anim_drink},
    {16, &anim_add_oil},
    {-1, NULL},
};

spi_oled_animation_t *get_animation_by_key(int key)
{
    for (int i = 0; animation_map[i].key != -1; i++)
    {
        if (animation_map[i].key == key)
        {
            printf("map found: %d , width: %d , height: %d\n",
                   animation_map[i].key,
                   animation_map[i].animation->width,
                   animation_map[i].animation->height);
            return animation_map[i].animation;
        }
    }

    printf("map not found\n");
    return NULL;
}
