/**
 * @file agc.h
 * @brief 播放链路自动增益控制参数和状态结构。
 */

#ifndef BBTALKIE_AGC_H
#define BBTALKIE_AGC_H

#ifdef __cplusplus
extern "C" {
#endif

#define TARGET_RMS 6000
#define AGC_ATTACK 0.1f
#define AGC_RELEASE 0.01f
#define MIN_GAIN 0.1f
#define MAX_GAIN 8.0f

typedef struct
{
    float current_gain;
    float target_rms;
    float attack_rate;
    float release_rate;
} agc_t;

#ifdef __cplusplus
}
#endif

#endif /* BBTALKIE_AGC_H */
